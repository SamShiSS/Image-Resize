// Resize a BMP file with a factor of f

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "bmp.h"

LONG gcd(LONG a, LONG b);

int main(int argc, char *argv[])
{
    // ensure proper usage
    if (argc != 4 || isnan(atof(argv[1])) != 0)
    {
        fprintf(stderr, "Usage: resize f infile outfile\n");
        return 1;
    }

    // remember resize factor
    double f = atof(argv[1]);

    // remember filenames
    char *infile = argv[2];
    char *outfile = argv[3];

    // open input file
    FILE *inptr = fopen(infile, "r");
    if (inptr == NULL)
    {
        fprintf(stderr, "Could not open %s.\n", infile);
        return 1;
    }

    // open output file
    FILE *outptr = fopen(outfile, "w");
    if (outptr == NULL)
    {
        fclose(inptr);
        fprintf(stderr, "Could not create %s.\n", outfile);
        return 1;
    }

    // read infile's BITMAPFILEHEADER
    BITMAPFILEHEADER bf;
    fread(&bf, sizeof(BITMAPFILEHEADER), 1, inptr);

    // read infile's BITMAPINFOHEADER
    BITMAPINFOHEADER bi;
    fread(&bi, sizeof(BITMAPINFOHEADER), 1, inptr);

    // ensure infile is (likely) a 24-bit uncompressed BMP 4.0
    if (bf.bfType != 0x4d42 || bf.bfOffBits != 54 || bi.biSize != 40 ||
        bi.biBitCount != 24 || bi.biCompression != 0)
    {
        fclose(outptr);
        fclose(inptr);
        fprintf(stderr, "Unsupported file format.\n");
        return 1;
    }

    // save infile's width and height
    LONG w_old = bi.biWidth;
    LONG h_old = abs(bi.biHeight);

    // determine padding for infile's scanlines
    int padding = (4 - (w_old * sizeof(RGBTRIPLE)) % 4) % 4;

    // declare array of RGBTRIPLE to save infile's colours
    RGBTRIPLE triple_old[h_old][w_old];

    // iterate over infile's scanlines, and save it in the array
    for (int i = 0; i < h_old; i++)
    {
        // iterate over pixels in scanline
        for (int j = 0; j < w_old; j++)
        {
            // read RGB triple from infile
            fread(&triple_old[i][j], sizeof(RGBTRIPLE), 1, inptr);
        }
        // skip over padding, if any
        fseek(inptr, padding, SEEK_CUR);
    }
    // close infile
    fclose(inptr);

    // modify outfile's width and height in BITMAPINFOHEADER
    bi.biWidth = round(bi.biWidth * f);
    bi.biHeight = round(bi.biHeight * f);

    // assign new variables for outfile's width and height for programmer's convenience
    LONG w_new = bi.biWidth;
    LONG h_new = abs(bi.biHeight);

    // determine padding for outfile
    padding = (4 - (w_new * sizeof(RGBTRIPLE)) % 4) % 4;

    // modify outfile's file size and image size
    bf.bfSize = 54 + (3 * w_new + padding) * h_new;
    bi.biSizeImage = (3 * w_new + padding) * h_new;

    // write outfile's BITMAPFILEHEADER
    fwrite(&bf, sizeof(BITMAPFILEHEADER), 1, outptr);

    // write outfile's BITMAPINFOHEADER
    fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, outptr);

    // find least common multipliers between the old and new width, and between old and new height
    LONG w_lcm = w_old * w_new / gcd(w_old, w_new);
    LONG h_lcm = h_old * h_new / gcd(h_old, h_new);

    // width and height of boxes in each pixel of the new image
    LONG w_box = w_lcm / w_new;
    LONG h_box = h_lcm / h_new;

    // declare RGBTRIPLE to save outfile's colours
    RGBTRIPLE triple_new;

    // declare float variables for pixel colour calculation
    int totalBlue, totalGreen, totalRed;

    // interating over each pixel in the new image, then
    for (int i = 0; i < h_new; i++)
    {
        // iterating over columns of least common multiplier between old and new width
        for (int j = 0; j < w_new; j++)
        {
            // initialize the RGB colour of each pixel to zero
            totalBlue = 0;
            totalGreen = 0;
            totalRed = 0;
            // iterating over each box in the pixel, for each box, infer the number of old pixel it belongs to
            // iterating over each row of boxex in the pixel
            for (int k = 0; k < h_box; k++)
            {
                // iterating over each box in the row
                for (int l = 0; l < w_box; l++)
                {
                    // extract the RGB colour for this box from the old pixel and add to the total RGB colour of the pixel
                    totalBlue += triple_old[(i * w_box + k) / (w_lcm / w_old)][(j * h_box + l) / (h_lcm / h_old)].rgbtBlue;
                    totalGreen += triple_old[(i * w_box + k) / (w_lcm / w_old)][(j * h_box + l) / (h_lcm / h_old)].rgbtGreen;
                    totalRed += triple_old[(i * w_box + k) / (w_lcm / w_old)][(j * h_box + l) / (h_lcm / h_old)].rgbtRed;
                }
            }
            // insert RGB colour into the new pixel, and divide by the number of boxes in the pixel
            triple_new.rgbtBlue = (float)totalBlue / (h_box * w_box);
            triple_new.rgbtGreen = (float)totalGreen / (h_box * w_box);
            triple_new.rgbtRed = (float)totalRed / (h_box * w_box);

            // write RGB colour to outfile
            fwrite(&triple_new, sizeof(RGBTRIPLE), 1, outptr);
        }
        // add padding
        for (int m = 0; m < padding; m++)
        {
            fputc(0x00, outptr);
        }
    }
    // close outfile
    fclose(outptr);

    // success
    return 0;
}

LONG gcd(LONG a, LONG b)
{
    if (a == 0)
    {
        return b;
    }
    return gcd(b % a, a);
}