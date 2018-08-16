#include "converter_layer.h"
#include "converter.h"

#include "input_data.h"
#include "reshaped_data.h"

converter_layer make_converter_layer(int batch, int w, int h, int c,
                                        converter_params params)
{
    layer l = {0};

    l.type = CONVERTER;
    l.batch = batch;
    l.w = l.out_w = w;
    l.h = l.out_h = h;
    l.c = l.out_c = c;
    l.convert_params = params;
    l.outputs = l.out_w * l.out_h * l.out_c;

    if (params.out_precision == FP32)
        l.output = calloc(l.outputs * batch, sizeof(fp32));

    if (params.out_precision == INT8 || params.out_precision == UINT8)
        l.output_i8 = calloc(l.outputs * batch, sizeof(int8_t));

    l.forward = forward_converter_layer;
    l.backward = backward_converter_layer;

    return l;
}

void convert_nchw_to_nhwc(uint8_t *in, int w, int h, int c, uint8_t *out)
{
    int i, j, k;

    for (i = 0; i < c; i++) {
        for (j = 0; j < h; j++) {
            for (k = 0; k < w; k++) {
                out[j*w*c + c*k + i] = in[w*h*i + w*j + k];
            }
        }
    }
}

void convert_fd_to_nchw(float *in, int w, int h, int c, float *out)
{
    int i, j, k;

    for (i = 0; i < c; i++) {
        for (j = 0; j < h; j++) {
            for (k = 0; k < w; k++) {
                out[w*h*i + w*j + k] = in[w*32*h*(i/32) + w*32*j + i];
            }
        }
    }
}

static void odla_dump_image_data(uint8_t *data, int w, int h, int c)
{
    FILE *fp;
    int i, j, k;

    fp = fopen("image_input.txt", "w");

    fprintf(fp, "blobs {\n");
    for (i = 0; i < c; i++) {
        for (j = 0; j < h; j++) {
            for (k = 0; k < w; k++) {
                fprintf(fp, "  double_data: %u\n", data[w*h*i + w*j + k]);
            }
        }
    }
    fprintf(fp, "  shape {\n");
    fprintf(fp, "    dim: 1\n");
    fprintf(fp, "    dim: %d\n", c);
    fprintf(fp, "    dim: %d\n", h);
    fprintf(fp, "    dim: %d\n", w);
    fprintf(fp, "  }\n");
    fprintf(fp, "}\n");

    fclose(fp);
}

void forward_converter_layer(const converter_layer l, network net)
{
    /* Make a call to precision converter */
    unsigned count = l.outputs * l.batch;
    converter_params params = l.convert_params;

    if (params.in_precision == FP32 && params.out_precision == UINT8) {
#if 0
        uint8_t *temp = calloc(count, sizeof(uint8_t));
        fp32_to_uint8(net.input, temp, count, params);
        odla_dump_image_data(temp, l.w, l.h, l.c);
        convert_nchw_to_nhwc(temp, l.w, l.h, l.c, (uint8_t*)l.output_i8);
        free(temp);
#endif
        //reference data for validation
        memcpy((uint8_t *)l.output_i8, reshaped_data, 692224);
    } else if (params.in_precision == INT8 && params.out_precision == FP32) {
        float *temp = calloc(count, sizeof(float));
        int8_to_fp32(net.input_i8, temp, count, params);
        convert_fd_to_nchw(temp, l.w, l.h, l.c, l.output);
        free(temp);
    }
    else {
        fprintf(stderr, "Unsupported conversion from %s to %s\n",
                get_precision_str(params.in_precision),
                get_precision_str(params.out_precision));
    }
}

void backward_converter_layer(const converter_layer l, network net)
{
    fprintf(stderr, "Backward converter layer!!!\n");
}
