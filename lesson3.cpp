#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "bitreader.h"
#include "bitwriter.h"
#include "test_harness.h"
#include "dyn_prob.h"

/////////////////
//// Lesson 3 - Numeric Encodings
/////////////////
/*
  This lesson illustrates that a righer set of probabilities can be used to better describe a number
  If we track which bit we're encoding and all the previously observed bits for the number so far
  We can rapidly hone in on which number is being encoded currently and throw out numbers
  That might not be high probability outcomes for a particular coder state

  In arithmetic coding, binary or 2's complement encodings rarely tend to work well to train
  Arithmetic coders.

  The EXERCISE hands out a stream of exponentially decaying numbers.
  Instead of storing these using binary, it might make sense to code them using UNARY or
  Golomb codes.

  Unary codes equal a number of one's equal to the value followed by a trailing zero to indicate
  that it is done.

  Golomb coding is similar to how protocol buffers have a base 128 encoding with a stop bit.

 */



void encode_with_adaptive_probability() {
    DynProb encode[256];// one probability per number encoded so far and bit position
    vpx_writer wri ={0};
    vpx_start_encode(&wri, compressed);
    for (size_t i = 0; i < sizeof(uncompressed); ++i) {
        uint8_t encoded_so_far = 0;
        for(int bit = 128; bit > 0; bit >>= 1) {
            bool cur_bit = !!(uncompressed[i] & bit);
            vpx_write(&wri, cur_bit, encode[encoded_so_far + bit].prob);
            encode[encoded_so_far + bit].record_bit(cur_bit);
            encoded_so_far |= uncompressed[i] & bit;
        }
    }
    vpx_stop_encode(&wri);
    printf("Buffer encoded dynamically results in %d size (%.2f%%)\n",
           wri.pos,
           100 * wri.pos / float(sizeof(uncompressed)));
    DynProb decode[256];
    vpx_reader rea={0};
    vpx_reader_init(&rea,
                    wri.buffer,
                    wri.pos);
    memset(roundtrip, 0, sizeof(roundtrip));
    for (size_t i = 0; i < sizeof(roundtrip); ++i) {
        uint8_t decoded_so_far = 0;
        for(int bit = 128; bit > 0; bit >>= 1) {
            if (vpx_read(&rea, decode[decoded_so_far + bit].prob)) {
                roundtrip[i] |= bit;
                decode[decoded_so_far + bit].record_bit(true);
                decoded_so_far |= bit;
            } else {
                decode[decoded_so_far + bit].record_bit(false);
            }
        }
    }
    assert(vpx_reader_has_error(&rea) == 0);
    assert(memcmp(uncompressed, roundtrip, sizeof(uncompressed)) == 0);
}

int main () {
    printf("Random data\n");
    populate_random_data();
    encode_with_adaptive_probability();
    printf("ASCII data\n");
    populate_ascii_data();
    encode_with_adaptive_probability();
    printf("ZERO data\n");
    populate_zero_data();
    encode_with_adaptive_probability();
    printf("0, 1, or 3\n");
    populate_small_data();
    encode_with_adaptive_probability();
    printf("even = 0, odd is 1, or 3\n");
    populate_even_data();
    encode_with_adaptive_probability();
    printf("----------------\nEXERCISE\n");
    // can you come up with a different numeric encoding here?
    // base two may not be ideal for this sort of exponential skew
    // maybe something closer to golomb coding, or unary is better?
    exercise_zero_through_11ish_exp_skew();
    encode_with_adaptive_probability();
    return roundtrip[0];
}
