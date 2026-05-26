#include "sps_parser.h"

#include <assert.h>

static const uint8_t h264_test_data[] = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x2a,
        0xac, 0x2b, 0x40, 0x3c, 0x01, 0x13, 0xf2, 0xe0,
        0x2d, 0x41, 0x81, 0x81, 0xa9, 0x40, 0x00, 0x00,
        0xfa, 0x00, 0x00, 0x75, 0x30, 0x23, 0xc7, 0x0a,
        0xa8
};

static const uint8_t h265_test_data[] = {
        0x00, 0x00, 0x00, 0x01, 0x42, 0x01, 0x01, 0x21,
        0x40, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
        0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x7B, 0xA0,
        0x03, 0xC0, 0x80, 0x11, 0x07, 0xCB, 0x96, 0xB4,
        0xA4, 0x21, 0x19, 0x2E, 0x30, 0x16, 0xA0, 0xC0,
        0xC0, 0xD4, 0x82, 0x00, 0x00, 0x03, 0x00, 0x02,
        0x00, 0x00, 0x03, 0x00, 0x78, 0x5F, 0x1A, 0x2D
};

void test_sps_parse_dimension_h264(void) {
    sps_dimension_t dimension;
    assert(sps_parse_dimension_h264(&h264_test_data[4], &dimension));
    assert(1920 == dimension.width);
    assert(1080 == dimension.height);
}

void test_sps_parse_dimension_hevc(void) {
    sps_dimension_t dimension;
    assert(sps_parse_dimension_hevc(&h265_test_data[4], &dimension));
    assert(1920 == dimension.width);
    assert(1080 == dimension.height);
}

/* Regression: max_sub_layers_minus1 is a 3-bit field (0..7) but the spec only allows 0..6.
 * The parser previously sized sub_layer_*_present_flag[] to 6, so a malformed stream with
 * value 7 caused a 1-element OOB stack write. With arrays grown to [8] the parse must
 * complete cleanly (returning false because the rest of the crafted stream is invalid)
 * without writing past the buffer; under ASan the pre-fix code aborts here. */
void test_sps_parse_dimension_hevc_max_sub_layers_7(void) {
    static const uint8_t data[] = {
            0x0E,                                                              /* vps_id=0, max_sub_layers_minus1=7, tnf=0 */
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* profile_info (88 bits) */
            0x00,                                                              /* level_idc */
            0x00, 0x00,                                                        /* 14 sub_layer present pairs + 2 padding */
            0xFF,                                                              /* trailing 1-bits so subsequent ue(v) reads terminate */
    };
    sps_dimension_t dimension;
    assert(!sps_parse_dimension_hevc(data, &dimension));
}

// not needed when using generate_test_runner.rb
int main(void) {
    test_sps_parse_dimension_h264();
    test_sps_parse_dimension_hevc();
    test_sps_parse_dimension_hevc_max_sub_layers_7();
    return 0;
}