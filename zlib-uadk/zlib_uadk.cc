/* *********************************************************************
  Copyright (c) 2023 Huawei Technologies Co., Ltd.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Huawei Technologies Co. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************* */
#include <iostream>
#include <math.h>
#include <numa.h>
#include <sched.h>
#include <stdlib.h>
#include "wd_alg_common.h"
#include "wd_comp.h"
#include "wd_sched.h"
#include "zlib.h"
using namespace std;

const char *SCHED_RR_NAME = "sched_rr";
const int CTX_TYPE_COMPRESS = 0;
const int CTX_TYPE_DECOMPRESS = 1;
const int CTX_TYPE_NUM = 2;
const int UADK_INIT_OK = 0;
const int UADK_INIT_SCHED_RR_ALLOC_ERROR = 1;
const int UADK_INIT_SCHED_RR_INSTANCE_ERROR = 2;
const int UADK_INIT_REQUEST_CTX_ERROR = 3;
const int UADK_INIT_GET_DEV_LIST_ERROR = 4;
const int UADK_ALLOC_SESS_ERROR = 5;
const int UADK_MAX_BLOCK_SIZE = 1024 * 1024 * 6;
const int HEADER_SIZE = 10;
const int CHECK_HEADER_SIZE = 3;
const int UADK_OUTPUT_MAX_RETAIN = 16;
const int LEVEL_NUM = 10;
const unsigned char GZ_STD_HEADER[CHECK_HEADER_SIZE] = { 0x1f, 0x8b, 0x8 };
const wd_comp_level LEVEL_ARR[LEVEL_NUM] = {
    WD_COMP_L1, WD_COMP_L2, WD_COMP_L3, WD_COMP_L4, WD_COMP_L5,
    WD_COMP_L6, WD_COMP_L7, WD_COMP_L8, WD_COMP_L9, WD_COMP_L10
};
struct wd_ctx_config g_ctx;
struct wd_sched *g_sched;
char g_comp_retain[16];

// 定义UADK加速器压缩/解压状态
enum InnerStreamStatus {
    // 新创建或者重置session后获得的状态
    STREAM_NEW,
    // 如果单次压缩或解压后，当前并没有处理完所有的输入数据，则设置为此状态
    STREAM_WORKING,
    // 如果单次压缩或解压后，当前所有输入数据已经处理完成，但是并没有输出所有的输出数据，则设置为此状态
    STREAM_OUTPUT_REMAIN,
    // 如果单次压缩或解压后，当前所有输入数据已经处理完成，并且所有输出数据输出完成，则设置为此状态
    STREAM_END,
};
using StreamStatus = InnerStreamStatus;

// 在UDAK实现中，internal_state的实现
struct internal_state {
    // uadk加速器使用的session
    handle_t handle;
    // 内部输入缓冲区
    uint8_t *inputBuffer;
    // 内部输出缓冲区
    uint8_t *outputBuffer;
    // 内部输入缓冲区大小
    unsigned int inputBufferSize;
    // 内部输入缓冲区中输入数据大小
    unsigned int inputDataSize;
    // 内部输出缓冲区大小
    unsigned int outputBufferSize;
    // 内部输出缓冲区已输出到用户空间的数据大小
    unsigned int outputConsumed;
    // 内部输出缓冲区的输出数据大小
    unsigned int outputDataSize;
    // UADK加速器的操作类型（压缩或解压缩）
    wd_comp_op_type opType;
    // UADK加速器压缩/解压状态
    StreamStatus status;
    // 为了解决解压过程中未消费完输出的情况下把输入消费完的问题，需要在未消费完输出的情况下需要在输入端预留至少1个字节的数据，并在此字段做记录，下次执行inflate的时候跳过这些预留的数据
    unsigned char nextSkipBytes;
    unsigned char accelTimeoutTime;
};
using CompressionHandle = internal_state;

// uadk加速器调度器初始化
const int UADK_MODE_NUM = 2;
int UadkCompSchedInit(int *qStart, int *qEnd, struct wd_sched **schedp)
{
    struct sched_params param;
    struct wd_sched *sched;
    int i, ret;
    // 调度器空间初始化
    sched = wd_sched_rr_alloc(SCHED_POLICY_RR, UADK_MODE_NUM, 1, wd_comp_poll_ctx);
    if (sched == nullptr) {
        return UADK_INIT_SCHED_RR_ALLOC_ERROR;
    }
    sched->name = SCHED_RR_NAME;
    // 对压缩和解压缩设置调度规则
    for (i = 0; i < CTX_TYPE_NUM; i++) {
        // 代表异步或者同步（这里只使用同步，因此只取0）
        param.mode = 0;
        // 操作类型，0为压缩，1为解压
        param.type = i;
        // numa_id，这里不考虑分numa分配的情形，因此统一设置为0
        param.numa_id = 0;
        // 设置调度规则可选择的ctx的起始位置和结束位置，当创建session的时候，调度器会从这个范围内选取合适的ctx
        param.begin = qStart[i];
        param.end = qEnd[i] - 1;
        // 创建调度规则
        ret = wd_sched_rr_instance(sched, &param);
        if (ret < 0) {
            goto out_free_sched;
        }
    }
    *schedp = sched;
    return ret;

out_free_sched:
    wd_sched_rr_release(sched);
    return ret;
}

// 根据算法名称获取加速器设备列表（一般是2个设备）
int GetDevList(const char *algName, struct uacce_dev_list ***ret)
{
    int devListNum = 0;
    struct uacce_dev_list *list, *p;
    list = wd_get_accel_list(algName);
    if (list == nullptr) {
        return 0;
    }
    // wd_get_accel_list获取的是链表，转换成数组方便获取
    p = list;
    while (p != nullptr) {
        devListNum++;
        p = p->next;
    }
    struct uacce_dev_list **devList = (struct uacce_dev_list **)malloc(devListNum * sizeof(struct uacce_dev_list *));
    p = list;
    int currIdx = 0;
    while (p != nullptr) {
        devList[currIdx++] = p;
        p = p->next;
    }
    *ret = devList;
    return devListNum;
}

// UADK加速器初始化流程
int UadkCompCtxInit(struct uacce_dev_list **devList, int devListNum, int compressCtxNum, int decompressCtxNum,
    int useSingleDev)
{
    int i, j, ret;
    // 根据传入的参数设置压缩和解压需要的ctx
    // 因为压缩/解压类型在UADK中分别为0和1，因此使用长度为2的数组设置相关的值，方便后续循环处理，减少代码量
    int qStart[CTX_TYPE_NUM] = { 0, compressCtxNum };
    // 因为压缩/解压用的ctx是放在同一个一维数组中的，因此需要确定压缩/解压所使用的ctx在一维数组的范围
    int qEnd[CTX_TYPE_NUM] = { compressCtxNum, compressCtxNum + decompressCtxNum };
    memset(&g_ctx, 0, sizeof(struct wd_ctx_config));
    g_ctx.ctx_num = qEnd[CTX_TYPE_DECOMPRESS];
    g_ctx.ctxs = (wd_ctx *)calloc(g_ctx.ctx_num, sizeof(struct wd_ctx));
    if (!g_ctx.ctxs) {
        return -WD_ENOMEM;
    }
    int numaNum = numa_max_node() + 1;
    int cpuNuma = numa_node_of_cpu(sched_getcpu());
    // uadk申请ctx，i的取值只有0或者1，代表压缩和解压
    for (i = 0; i < CTX_TYPE_NUM; i++) {
        // 之前定义的压缩/解压所需的ctx所放置的地址范围，对应申请的ctx只会放在对应的范围中
        for (j = qStart[i]; j < qEnd[i]; j++) {
            // 根据参数确定是否使用单个设备
            if (useSingleDev) {
                // 如果使用单个设备，则根据当前所使用的cpu所在numa选择合适的设备
                // 如果当前cpu在主片上，则使用index为0的设备，如果在从片上，则使用index为1的设备
                g_ctx.ctxs[j].ctx = wd_request_ctx(devList[cpuNuma / (numaNum / devListNum)]->dev);
            } else {
                // 不使用单个设备的话，会平均交叉申请两个设备的ctx
                g_ctx.ctxs[j].ctx = wd_request_ctx(devList[(j - qStart[i]) % devListNum]->dev);
            }
            if (!g_ctx.ctxs[j].ctx) {
                ret = UADK_INIT_REQUEST_CTX_ERROR;
                goto out_free_ctx;
            }
            // 设置mode，0为同步，1为异步(这里只使用同步，因此只取0)
            g_ctx.ctxs[j].ctx_mode = 0;
            // 操作类型，0为压缩，1为解压
            g_ctx.ctxs[j].op_type = i;
        }
    }
    // 配置ctx调度算法操作
    ret = UadkCompSchedInit(qStart, qEnd, &g_sched);
    if (ret) {
        goto out_free_ctx;
    }
    // 全局初始化
    ret = wd_comp_init(&g_ctx, g_sched);
    if (ret) {
        goto out_free_sched;
    }
    return UADK_INIT_OK;

// 初始化失败则需要释放调度算法
out_free_sched:
    wd_sched_rr_release(g_sched);
// 初始化失败则需要释放申请的ctx
out_free_ctx:
    for (i = 0; i < g_ctx.ctx_num; i++)
        if (g_ctx.ctxs[i].ctx)
            wd_release_ctx(g_ctx.ctxs[i].ctx);
    free(g_ctx.ctxs);
    return ret;
}

// 创建压缩/解压所需session
handle_t UadkCompSessInit(wd_comp_op_type type, int level)
{
    if (level >= LEVEL_NUM || level < 0) {
        return 0;
    }
    struct wd_comp_sess_setup setup;
    memset(&setup, 0, sizeof(struct wd_comp_sess_setup));
    struct sched_params param;
    memset(&param, 0, sizeof(struct sched_params));
    // 算法类型，固定为GZIP
    setup.alg_type = WD_GZIP;
    // 压缩或解压类型，根据传参确定
    setup.op_type = type;
    // 压缩级别
    setup.comp_lv = LEVEL_ARR[level];
    // 窗口大小，这里暂时固定
    setup.win_sz = WD_COMP_WS_32K;
    // 压缩或解压类型，根据传参确定
    param.type = type;
    // 代表异步或者同步（这里只使用同步，因此只取0）
    param.mode = 0;
    // numa_id，实际是在设置调度算法中配置的numa_id，因为只设置了0，因此这里只能填0
    param.numa_id = 0;
    setup.sched_param = &param;
    // 系统会根据上面设置的opType, mode和numa_id，在调度算法中选择对应范围的ctx中根据调度策略选择一个ctx
    handle_t ret = wd_comp_alloc_sess(&setup);
    return ret;
}

// 获取int类型的环境变量
int GetIntEnv(const char *envname, int def)
{
    char *env = secure_getenv(envname);
    if (env == nullptr) {
        return def;
    }
    int ret = atoi(env);
    if (ret != 0 || strcmp(env, "0") == 0)
        return ret;
    return def;
}

class UADKInit {
public:
    UADKInit()
    {
        int compressCtxNum = max(GetIntEnv("UADK_COMPRESS_CTX_NUM", 4), 1);
        int decompressCtxNum = max(GetIntEnv("UADK_DECOMPRESS_CTX_NUM", 4), 1);
        unsigned int useSingleDev = GetIntEnv("UADK_USE_SINGLE_DEV", 0);
        struct uacce_dev_list **devList = nullptr;
        int devListLen = GetDevList("gzip", &devList);
        if (devList == nullptr) {
            ret = UADK_INIT_GET_DEV_LIST_ERROR;
            return;
        }
        ret = UadkCompCtxInit(devList, devListLen, compressCtxNum, decompressCtxNum, useSingleDev);
        wd_free_list_accels(devList[0]);
    };
    int GetRet()
    {
        return ret;
    };
    ~UADKInit() {};

private:
    int ret;
};

static int Init()
{
    static UADKInit init;
    return init.GetRet();
}

int ResetHandle(CompressionHandle *handle)
{
    handle->inputDataSize = 0;
    handle->outputDataSize = 0;
    handle->outputConsumed = 0;
    handle->status = STREAM_NEW;
    handle->nextSkipBytes = 0;
    handle->accelTimeoutTime = 0;
    return wd_comp_reset_sess(handle->handle);
}

int InitHandle(CompressionHandle *ret, wd_comp_op_type opType, int useBuffer, unsigned int inputBufferSize,
    unsigned int outputBufferSize, int level)
{
    ret->opType = opType;
    if (useBuffer) {
        ret->inputBuffer = (uint8_t *)malloc(inputBufferSize);
        if (ret->inputBuffer == nullptr) {
            return UADK_ALLOC_SESS_ERROR;
        }
        ret->outputBuffer = (uint8_t *)malloc(outputBufferSize);
        if (ret->outputBuffer == nullptr) {
            free(ret->inputBuffer);
            return UADK_ALLOC_SESS_ERROR;
        }
        ret->inputBufferSize = inputBufferSize;
        ret->outputBufferSize = outputBufferSize;
    } else {
        ret->inputBuffer = nullptr;
        ret->outputBuffer = nullptr;
        ret->inputBufferSize = 0;
        ret->outputBufferSize = 0;
    }
    ret->handle = UadkCompSessInit(opType, level);
    if (ret->handle == 0) {
        return UADK_ALLOC_SESS_ERROR;
    }
    return ResetHandle(ret);
}

void FreeHandle(CompressionHandle *handle)
{
    if (handle->handle) {
        wd_comp_free_sess(handle->handle);
        handle->handle = 0;
    }
    if (handle->inputBuffer != nullptr) {
        free(handle->inputBuffer);
        handle->inputBuffer = nullptr;
    }
    if (handle->outputBuffer != nullptr) {
        free(handle->outputBuffer);
        handle->outputBuffer = nullptr;
    }
}

unsigned int GetCompInputBufferSize()
{
    static unsigned int ret = GetIntEnv("UADK_COMP_INPUT_BUFFER_SIZE", 64 * 1024);
    return ret;
}

unsigned int GetCompOutputBufferSize()
{
    static unsigned int ret = GetIntEnv("UADK_COMP_OUTPUT_BUFFER_SIZE", 64 * 1024);
    return ret;
}

unsigned int GetDecompInputBufferSize()
{
    static unsigned int ret = GetIntEnv("UADK_DECOMP_INPUT_BUFFER_SIZE", 64 * 1024);
    return ret;
}

unsigned int GetDecompOutputBufferSize()
{
    static unsigned int ret = GetIntEnv("UADK_DECOMP_OUTPUT_BUFFER_SIZE", 64 * 1024);
    return ret;
}

unsigned int UseCompBuffer()
{
    static unsigned int ret = GetIntEnv("UADK_USE_COMP_BUFFER", 1);
    return ret;
}

unsigned int UseDecompBuffer()
{
    static unsigned int ret = GetIntEnv("UADK_USE_DECOMP_BUFFER", 1);
    return ret;
}

unsigned int InflateForceFlush()
{
    static unsigned int ret = GetIntEnv("UADK_INFLATE_FORCE_FLUSH", 1);
    return ret;
}

unsigned int AccelTimeoutTimeBeforeExit()
{
    static unsigned int ret = GetIntEnv("UADK_ACCEL_TIMEOUT_TIME_BEFORE_EXIT", 0);
    return ret;
}

ZEXTERN int ZEXPORT deflateInit2_(z_streamp strm, int level, int method, int windowBits, int memLevel, int strategy,
    const char *version, int stream_size)
{
    if (strm == Z_NULL)
        return Z_STREAM_ERROR;
    strm->state = (CompressionHandle *)malloc(sizeof(CompressionHandle));
    if (strm->state == nullptr) {
        return Z_STREAM_ERROR;
    }
    int init_ret = Init();
    if (init_ret)
        return init_ret;
    int ret = InitHandle(strm->state, WD_DIR_COMPRESS, UseCompBuffer(), GetCompInputBufferSize(),
        GetCompOutputBufferSize(), level);
    if (ret) {
        free(strm->state);
    }
    return ret;
}

ZEXTERN int ZEXPORT deflateInit_(z_streamp strm, int level, const char *version, int stream_size)
{
    return deflateInit2_(strm, level, 0, 0, 0, 0, version, stream_size);
}

ZEXTERN int ZEXPORT inflateInit2_(z_streamp strm, int windowBits, const char *version, int stream_size)
{
    if (strm == Z_NULL)
        return Z_STREAM_ERROR;
    strm->state = (CompressionHandle *)malloc(sizeof(CompressionHandle));
    if (strm->state == nullptr) {
        return Z_STREAM_ERROR;
    }
    int init_ret = Init();
    if (init_ret)
        return init_ret;
    int ret = InitHandle(strm->state, WD_DIR_DECOMPRESS, UseDecompBuffer(), GetDecompInputBufferSize(),
        GetDecompOutputBufferSize(), 0);
    if (ret) {
        free(strm->state);
    }
    return ret;
}

ZEXTERN int ZEXPORT inflateInit_(z_streamp strm, const char *version, int stream_size)
{
    return inflateInit2_(strm, 0, version, stream_size);
}

ZEXTERN int ZEXPORT deflateReset(z_streamp strm)
{
    strm->total_in = strm->total_out = 0;
    strm->msg = Z_NULL;
    strm->data_type = Z_UNKNOWN;
    return ResetHandle(strm->state);
}

ZEXTERN int ZEXPORT inflateReset(z_streamp strm)
{
    return deflateReset(strm);
}

ZEXTERN int ZEXPORT deflateEnd(z_streamp strm)
{
    FreeHandle(strm->state);
    free(strm->state);
    return Z_OK;
}

ZEXTERN int ZEXPORT inflateEnd(z_streamp strm)
{
    return deflateEnd(strm);
}

void FillCompReq(struct wd_comp_req *req, void *src, void *dst, unsigned int srcLen, unsigned int dstLen,
    wd_comp_op_type opType, int last)
{
    req->src = src;
    req->dst = dst;
    req->src_len = srcLen;
    req->dst_len = dstLen;
    req->op_type = opType;
    req->data_fmt = WD_FLAT_BUF;
    req->cb = nullptr;
    req->last = last;
}

bool IsValidHeader(wd_comp_op_type opType, StreamStatus status, const unsigned char *data, unsigned int len, int *ret)
{
    if (opType == WD_DIR_COMPRESS || status == STREAM_WORKING || status == STREAM_OUTPUT_REMAIN) {
        return true;
    }
    if (len < HEADER_SIZE) {
        *ret = Z_OK;
        return false;
    }
    for (int i = 0; i < CHECK_HEADER_SIZE; i++) {
        if (GZ_STD_HEADER[i] != data[i]) {
            *ret = Z_DATA_ERROR;
            return false;
        }
    }
    return true;
}

bool ShouldCommitCompressionOp(z_streamp strm, int flush, void *src, void *dst, unsigned int srcLen,
    unsigned int dstLen, int *zStreamRet)
{
    if (dstLen <= UADK_OUTPUT_MAX_RETAIN) {
        *zStreamRet = Z_BUF_ERROR;
        return false;
    }
    CompressionHandle *handle = strm->state;
    if (srcLen > 0) {
        if (!IsValidHeader(handle->opType, handle->status, (const unsigned char *)src, srcLen, zStreamRet)) {
            return false;
        }
        if (src != handle->inputBuffer) {
            return true;
        }
        if (flush != Z_NO_FLUSH) {
            return true;
        }
        if (strm->avail_in > 0) {
            return true;
        }
        *zStreamRet = Z_OK;
        return false;
    }
    if (handle->status == STREAM_OUTPUT_REMAIN) {
        return true;
    }
    if (handle->status == STREAM_END) {
        *zStreamRet = Z_STREAM_END;
        return false;
    }
    if (handle->opType == WD_DIR_COMPRESS && flush == Z_FINISH) {
        return true;
    }
    *zStreamRet = Z_OK;
    return false;
}

ZEXTERN int ZEXPORT UadkCompressionOpDirect(z_streamp strm, int flush)
{
    CompressionHandle *handle = strm->state;
    int ret = Z_OK;
    if (!ShouldCommitCompressionOp(strm, flush, strm->next_in, strm->next_out, strm->avail_in, strm->avail_out, &ret))
        return ret;
    struct wd_comp_req req;
    FillCompReq(&req, strm->next_in, strm->next_out, strm->avail_in, strm->avail_out, handle->opType,
        flush == Z_FINISH && handle->opType == WD_DIR_COMPRESS);
    ret = wd_do_comp_strm(handle->handle, &req);
    if (ret) {
        if (ret == -ETIME && AccelTimeoutTimeBeforeExit() > 0) {
            handle->accelTimeoutTime++;
            if (handle->accelTimeoutTime >= AccelTimeoutTimeBeforeExit()) {
                return Z_STREAM_ERROR;
            }
        }
        return ret;
    }
    if (req.status == WD_EAGAIN) {
        handle->status = STREAM_OUTPUT_REMAIN;
        ret = Z_OK;
    } else if (req.status == WD_STREAM_END) {
        handle->status = STREAM_END;
        ret = Z_STREAM_END;
    } else if (req.status == WD_SUCCESS) {
        handle->status = STREAM_WORKING;
        ret = Z_OK;
    } else {
        return req.status;
    }
    strm->next_in += req.src_len;
    strm->avail_in -= req.src_len;
    strm->total_in += req.src_len;
    strm->next_out += req.dst_len;
    strm->avail_out -= req.dst_len;
    strm->total_out += req.dst_len;
    return ret;
}

bool TakeOutputBuffer(z_streamp strm)
{
    CompressionHandle *handle = strm->state;
    if (strm->state->outputConsumed < strm->state->outputDataSize) {
        uint32_t copyLen = min(strm->avail_out, strm->state->outputDataSize - strm->state->outputConsumed);
        memcpy(strm->next_out, strm->state->outputBuffer + strm->state->outputConsumed, copyLen);
        handle->outputConsumed += copyLen;
        strm->next_out += copyLen;
        strm->avail_out -= copyLen;
        strm->total_out += copyLen;
        return true;
    }
    return false;
}

bool SelectSrcBuffer(z_streamp strm, int flush, void **src, unsigned int *srcLen)
{
    CompressionHandle *handle = strm->state;
    bool useBufferAsInput = false;
    if (handle->inputDataSize > 0) {
        // when buffer input has data, use buffer as input.
        useBufferAsInput = true;
    } else if (*srcLen > UADK_MAX_BLOCK_SIZE) {
        useBufferAsInput = true;
    } else if (flush != Z_NO_FLUSH) {
        // otherwise:
        // when flush != Z_NO_FLUSH, use user input as input.
        useBufferAsInput = false;
    } else {
        // otherwise:
        // when buffer input size > user data size, use buffer as input.
        // otherwise use user input as input.
        useBufferAsInput = handle->inputBufferSize > strm->avail_in;
    }
    if (useBufferAsInput) {
        // copy input data from user input to input buffer
        uint32_t copyLen = min(strm->avail_in, strm->state->inputBufferSize - strm->state->inputDataSize);
        if (copyLen > 0) {
            memcpy(strm->state->inputBuffer + strm->state->inputDataSize, strm->next_in, copyLen);
            strm->total_in += copyLen;
            strm->avail_in -= copyLen;
            strm->next_in += copyLen;
            strm->state->inputDataSize += copyLen;
        }
        // use buffer as input
        *src = strm->state->inputBuffer;
        *srcLen = strm->state->inputDataSize;
    } else {
        // use user space as input
        *src = strm->next_in;
        *srcLen = strm->avail_in;
    }
    return useBufferAsInput;
}

bool SelectDstBuffer(z_streamp strm, void **dst, unsigned int *dstLen)
{
    CompressionHandle *handle = strm->state;
    bool useBufferAsOutput = *dstLen > UADK_MAX_BLOCK_SIZE ? true : (handle->outputBufferSize > strm->avail_out);
    if (useBufferAsOutput) {
        // use buffer as output
        *dst = strm->state->outputBuffer;
        *dstLen = strm->state->outputBufferSize;
    } else {
        // use user space as output
        *dst = strm->next_out;
        *dstLen = strm->avail_out;
    }
    return useBufferAsOutput;
}

ZEXTERN int ZEXPORT UadkCompressionOpBuffer(z_streamp strm, int flush)
{
    CompressionHandle *handle = strm->state;
    int oldAvailOut = strm->avail_out;
    int oldAvailIn = strm->avail_in;
    int retWhenNoProgress = Z_BUF_ERROR;
    strm->avail_in -= handle->nextSkipBytes;
    strm->next_in += handle->nextSkipBytes;
    handle->nextSkipBytes = 0;
    while (strm->avail_out > 0) {
        // copy all output buffer data to user output.
        if (TakeOutputBuffer(strm)) {
            continue;
        }
        // output buffer has no data and user output not full, require compression operation.
        handle->outputConsumed = handle->outputDataSize = 0;
        void *src = nullptr;
        void *dst = nullptr;
        unsigned int srcLen, dstLen;
        // when output buffer size > user output size, use buffer as input.
        // otherwise use user input as input.
        bool useBufferAsOutput = SelectDstBuffer(strm, &dst, &dstLen);
        bool useBufferAsInput = SelectSrcBuffer(strm, flush, &src, &srcLen);
        if (!ShouldCommitCompressionOp(strm, flush, src, dst, srcLen, dstLen, &retWhenNoProgress))
            break;
        int isLastData = (flush == Z_FINISH && handle->opType == WD_DIR_COMPRESS);
        if (useBufferAsInput) {
            isLastData = (isLastData && (strm->avail_in <= 0));
        }
        struct wd_comp_req req;
        FillCompReq(&req, src, dst, srcLen, dstLen, handle->opType, isLastData);
        retWhenNoProgress = wd_do_comp_strm(handle->handle, &req);
        if (retWhenNoProgress) {
            break;
        }
        // when compress, fixed req.status to WD_STREAM_END when no any input data remaining and compress success
        // because it is indicated that compress end reached.
        if (isLastData && req.status == 0 && req.src_len == srcLen) {
            req.status = WD_STREAM_END;
        }
        if (req.status == WD_EAGAIN) {
            handle->status = STREAM_OUTPUT_REMAIN;
        } else if (req.status == WD_STREAM_END) {
            handle->status = STREAM_END;
        } else if (req.status == WD_SUCCESS) {
            handle->status = STREAM_WORKING;
        } else {
            retWhenNoProgress = req.status;
            break;
        }
        retWhenNoProgress = Z_BUF_ERROR;
        if (useBufferAsInput) {
            if (req.src_len < handle->inputDataSize) {
                handle->inputDataSize = handle->inputDataSize - req.src_len;
                memcpy(handle->inputBuffer, handle->inputBuffer + req.src_len, handle->inputDataSize);
            } else {
                handle->inputDataSize = 0;
            }
        } else {
            strm->next_in += req.src_len;
            strm->avail_in -= req.src_len;
            strm->total_in += req.src_len;
        }
        if (useBufferAsOutput) {
            handle->outputDataSize = req.dst_len;
        } else {
            strm->next_out += req.dst_len;
            strm->avail_out -= req.dst_len;
            strm->total_out += req.dst_len;
        }
    }
    if (oldAvailOut == strm->avail_out && oldAvailIn == strm->avail_in) {
        if (retWhenNoProgress == -ETIME && AccelTimeoutTimeBeforeExit() > 0) {
            handle->accelTimeoutTime++;
            if (handle->accelTimeoutTime >= AccelTimeoutTimeBeforeExit()) {
                return Z_STREAM_ERROR;
            }
        }
        return retWhenNoProgress;
    }
    if (handle->opType == WD_DIR_DECOMPRESS && handle->status == STREAM_END && strm->avail_in == 0 &&
        (handle->outputDataSize > handle->outputConsumed || handle->inputDataSize > 0)) {
        strm->avail_in++;
        strm->next_in--;
        handle->nextSkipBytes = 1;
    }
    return (handle->status == STREAM_END && handle->outputDataSize == handle->outputConsumed) ? Z_STREAM_END : Z_OK;
}

ZEXTERN int ZEXPORT deflate(z_streamp strm, int flush)
{
    return UseCompBuffer() ? UadkCompressionOpBuffer(strm, flush) : UadkCompressionOpDirect(strm, flush);
}

ZEXTERN int ZEXPORT inflate(z_streamp strm, int flush)
{
    int tmpFlush = flush;
    if (InflateForceFlush()) {
        tmpFlush = Z_FULL_FLUSH;
    }
    return UseDecompBuffer() ? UadkCompressionOpBuffer(strm, tmpFlush) : UadkCompressionOpDirect(strm, tmpFlush);
}

ZEXTERN int ZEXPORT deflateSetDictionary(z_streamp strm, const Bytef *dictionary, uInt dictLength)
{
    return 0;
}

ZEXTERN int ZEXPORT inflateSetDictionary(z_streamp strm, const Bytef *dictionary, uInt dictLength)
{
    return 0;
}

int sync_compression_op(wd_comp_op_type opType, Bytef *dest, uLongf *destLen, const Bytef *source, uLong *sourceLen)
{
    if (Init()) {
        return Z_STREAM_ERROR;
    }
    handle_t session = UadkCompSessInit(opType, 6);
    struct wd_comp_req req;
    struct wd_datalist srclist, dstlist, retainlist;
    srclist.next = nullptr;
    dstlist.next = &retainlist;
    retainlist.data = g_comp_retain;
    retainlist.len = sizeof(g_comp_retain);
    retainlist.next = nullptr;
    req.op_type = opType;
    req.data_fmt = WD_SGL_BUF;
    req.cb = nullptr;
    srclist.data = (void *)source;
    srclist.len = *sourceLen;
    dstlist.data = (void *)dest;
    dstlist.len = *destLen;
    req.list_src = &srclist;
    req.list_dst = &dstlist;
    req.src_len = srclist.len;
    req.dst_len = dstlist.len + sizeof(g_comp_retain);
    req.last = (opType == WD_DIR_COMPRESS ? 1 : 0);
    int ret = wd_do_comp_sync(session, &req);
    wd_comp_free_sess(session);
    if (ret) {
        return Z_STREAM_ERROR;
    }
    if (req.status == WD_EAGAIN || req.dst_len > *destLen) {
        return Z_BUF_ERROR;
    }
    if (req.status == WD_SUCCESS) {
        if (req.src_len != *sourceLen) {
            return (opType == WD_DIR_DECOMPRESS ? Z_DATA_ERROR : Z_STREAM_ERROR);
        }
    } else if (req.status != WD_STREAM_END) {
        return Z_STREAM_ERROR;
    }
    *sourceLen = req.src_len;
    *destLen = req.dst_len;
    return Z_OK;
}

int ZEXPORT compress2(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level)
{
    if (sourceLen <= UADK_MAX_BLOCK_SIZE && *destLen <= UADK_MAX_BLOCK_SIZE) {
        return sync_compression_op(WD_DIR_COMPRESS, dest, destLen, source, &sourceLen);
    }
    z_stream stream;
    int err;
    uLong left;

    left = *destLen;

    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;
    stream.opaque = (voidpf)0;

    err = deflateInit(&stream, level);
    if (err != Z_OK) {
        return err;
    }

    stream.next_out = dest;
    stream.next_in = (z_const Bytef *)source;
    do {
        stream.avail_out = left > UADK_MAX_BLOCK_SIZE ? UADK_MAX_BLOCK_SIZE : (uInt)left;
        stream.avail_in = sourceLen > UADK_MAX_BLOCK_SIZE ? UADK_MAX_BLOCK_SIZE : (uInt)sourceLen;
        uInt oldIn = stream.avail_in;
        uInt oldOut = stream.avail_out;
        err = deflate(&stream, sourceLen ? Z_NO_FLUSH : Z_FINISH);
        left -= oldOut - stream.avail_out;
        sourceLen -= oldIn - stream.avail_in;
    } while (err == Z_OK);
    *destLen -= left;
    deflateEnd(&stream);
    return err == Z_STREAM_END ? Z_OK : err;
}

int ZEXPORT compress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
{
    return compress2(dest, destLen, source, sourceLen, Z_DEFAULT_COMPRESSION);
}

int ZEXPORT uncompress2(Bytef *dest, uLongf *destLen, const Bytef *source, uLong *sourceLen)
{
    if (*sourceLen <= UADK_MAX_BLOCK_SIZE && *destLen <= UADK_MAX_BLOCK_SIZE) {
        return sync_compression_op(WD_DIR_DECOMPRESS, dest, destLen, source, sourceLen);
    }
    z_stream stream;
    int err;
    uLong len, left;
    Byte buf[1];

    len = *sourceLen;
    if (*destLen) {
        left = *destLen;
    } else {
        left = 1;
        *destLen = left;
        dest = buf;
    }
    stream.next_in = (z_const Bytef *)source;
    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;
    stream.opaque = (voidpf)0;

    err = inflateInit(&stream);
    if (err != Z_OK) {
        return err;
    }
    stream.next_out = dest;

    do {
        stream.avail_out = left > UADK_MAX_BLOCK_SIZE ? UADK_MAX_BLOCK_SIZE : (uInt)left;
        stream.avail_in = len > UADK_MAX_BLOCK_SIZE ? UADK_MAX_BLOCK_SIZE : (uInt)len;
        uInt oldIn = stream.avail_in;
        uInt oldOut = stream.avail_out;
        err = inflate(&stream, Z_NO_FLUSH);
        left -= oldOut - stream.avail_out;
        len -= oldIn - stream.avail_in;
    } while (err == Z_OK);

    *sourceLen -= len;
    if (dest != buf) {
        *destLen -= left;
    } else if (stream.total_out && err == Z_BUF_ERROR) {
        left = 1;
    }
    inflateEnd(&stream);
    return err == Z_STREAM_END ?
        Z_OK :
        err == Z_NEED_DICT ? Z_DATA_ERROR : err == Z_BUF_ERROR && left + stream.avail_out ? Z_DATA_ERROR : err;
}

int ZEXPORT uncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
{
    return uncompress2(dest, destLen, source, &sourceLen);
}

const char *zError(int err)
{
    return "Error\n";
}

int gzcolse(gzFile file)
{
    return 0;
}