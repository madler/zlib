#ifndef CRC32_X86_HOOKS_H
#define CRC32_X86_HOOKS_H

/**
 * CRC HOOKS
 */
ZLIB_INTERNAL extern unsigned long (*crc32_z_hook)(unsigned long crc, const unsigned char FAR *buf, z_size_t len);

#endif /* CRC32_X86_HOOKS_H */
