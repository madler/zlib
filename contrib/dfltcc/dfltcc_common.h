#ifndef Z_DFLTCC_COMMON_H
#define Z_DFLTCC_COMMON_H

/*
   Runtime Tuning parameters.
*/

/*
  control usage of DFLTCC
  
  env variable: DFLTCC
  0 or unset: use DFLTCC algorithm (default)
  1: disable DFLTCC
 */
extern int env_dfltcc_disabled;
#define DFLTCC_DISABLED 0

/*
  control usage of DFLTCC_CMPR 
  set this to 1 if you need reproducible builds 

  env variable: SOURCE_DATE_EPOCH
  0 or unset: use DFLTCC_CMPR
  1: disable DFLTCC_CMPR
 */
extern int env_dfltcc_source_date_epoch;
#define DFLTCC_SOURCE_DATE_EPOCH 0

/*
  disable DFLTCC for specific compression levels

  env variable: DFLTCC_LEVEL_MASK
  valid range: 0-10 / 0x0-0xA
  default: 0x2 - disable for compresion level 0 to 2
 */
extern unsigned long env_dfltcc_level_mask;
#ifndef DFLTCC_LEVEL_MASK /* can be defined by configure */
#define DFLTCC_LEVEL_MASK 0x2
#endif
/*
  New block each X bytes

  env variable: DFLTCC_BLOCK_SIZE
  valid range: > 262144 / 0x40000 (256K)
  default: 1048576 / 0x100000 (1M)
 */
extern unsigned long env_dfltcc_block_size;
#define DFLTCC_BLOCK_SIZE 0x100000

/*
  New block after total_in > X

  env variable: DFLTCC_FIRST_FHT_BLOCK_SIZE
  default: 4096 / 0x1000
 */
extern unsigned long env_dfltcc_block_threshold;
#define DFLTCC_BLOCK_THRESHOLD 0x1000

/*
  New block only if avail_in >= X

  env variable: DFLTCC_DHT_MIN_SAMPLE_SIZE
  default: 4096 / 0x1000
*/
extern unsigned long env_dfltcc_dht_threshold;
#define DFLTCC_DHT_THRESHOLD 0x1000

/*
  default value for DFLTCC_RIBM register

  env variable: DFLTCC_RIBM
  default: 0
  valid range: 0-255 / 0x0-0xFF
*/
extern unsigned long env_dfltcc_ribm;
#define DFLTCC_RIBM 0


#endif
