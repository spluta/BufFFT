#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

struct NessStretchR;

extern "C" {

NessStretchR *ness_stretch_new(float dur_mult,
                               int32_t max_win_size,
                               int32_t win_size_divisor,
                               int32_t num_slices,
                               int32_t extreme,
                               int32_t paul_stretch_win_size);

void ness_stretch_free(NessStretchR *ptr);

void ness_stretch_next(NessStretchR *ptr, float *out_buf, int16_t size_out, int32_t off_set);

void ness_stretch_move_in_to_stored(NessStretchR *ptr);

void ness_stretch_set_in_chunk(NessStretchR *ptr, float value, int32_t index);

void ness_stretch_calc(NessStretchR *ptr);

} // extern "C"
