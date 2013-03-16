
#ifndef _IEEEHALFPRECISION_H_
#define _IEEEHALFPRECISION_H_

#ifdef __cplusplus
extern "C" {
#endif

// Prototypes -----------------------------------------------------------------

int singles2halfp(void *target, void *source, int numel);
int doubles2halfp(void *target, void *source, int numel);
int halfp2singles(void *target, void *source, int numel);
int halfp2doubles(void *target, void *source, int numel);

#ifdef __cplusplus
};
#endif

#endif
