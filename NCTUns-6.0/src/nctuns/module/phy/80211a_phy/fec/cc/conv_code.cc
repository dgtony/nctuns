/*
 * Copyright (c) from 2000 to 2009
 * 
 * Network and System Laboratory 
 * Department of Computer Science 
 * College of Computer Science
 * National Chiao Tung University, Taiwan
 * All Rights Reserved.
 * 
 * This source code file is part of the NCTUns 6.0 network simulator.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation is hereby granted (excluding for commercial or
 * for-profit use), provided that both the copyright notice and this
 * permission notice appear in all copies of the software, derivative
 * works, or modified versions, and any portions thereof, and that
 * both notices appear in supporting documentation, and that credit
 * is given to National Chiao Tung University, Taiwan in all publications 
 * reporting on direct or indirect use of this code or its derivatives.
 *
 * National Chiao Tung University, Taiwan makes no representations 
 * about the suitability of this software for any purpose. It is provided 
 * "AS IS" without express or implied warranty.
 *
 * A Web site containing the latest NCTUns 6.0 network simulator software 
 * and its documentations is set up at http://NSL.csie.nctu.edu.tw/nctuns.html.
 *
 * Project Chief-Technology-Officer
 * 
 * Prof. Shie-Yuan Wang <shieyuan@csie.nctu.edu.tw>
 * National Chiao Tung University, Taiwan
 *
 * 09/01/2009
 */

#include <cstdio>
#include <cmath>
#include "conv_code.h"

ConvoCode_11a::ConvoCode_11a(int _GX, int _GY, int _GLen, int _PX, int _PY,
		     int _PLen)
{
	int size = (int) pow(2, _GLen - 1);	// Number of states

	GX = _GX;
	GY = _GY;
	GLen = _GLen;
	PX = _PX;
	PY = _PY;
	PLen = _PLen;
	encoderTableSize = (int) pow(2, GLen);
	decoderTableSize = (int) pow(2, GLen + 1);
	decoderState = new int[size];

	resetEncoderState();
	resetDecoderState();
}

ConvoCode_11a::~ConvoCode_11a()
{
	delete decoderState;
}

struct EncoderLookupEntry *ConvoCode_11a::genEncoderLookupTable()
{
	struct EncoderLookupEntry *lookupTable;
	int x, y;
	int nextState;

	//
	// Use generator polynomials (GX and GY) to derive encoderLookupTable.
	// The index to this table is the 1-bit input together with (GLen-1)
	// 1-bit shift registers. Thus encoderTableSize should be 2^GLen.
	//
	lookupTable = new struct EncoderLookupEntry[encoderTableSize];

	for (int i = 0; i < encoderTableSize; i++) {
		x = BITCOUNT(i & GX) % 2;
		y = BITCOUNT(i & GY) % 2;
		nextState = (i >> 1);

		lookupTable[i].x = x;
		lookupTable[i].y = y;
		lookupTable[i].nextState = nextState;
	}

	return lookupTable;
}

// Make sure that the encoder lookup table is generated before this function
// gets called.
struct DecoderLookupEntry *ConvoCode_11a::genDecoderLookupTable()
{
	struct DecoderLookupEntry *lookupTable;
	int x, y;

	//
	// Use generator polynomials (GX and GY) and encoderLookupTable to
	// derive decoderLookupTable. The index to this table is the 2-bit
	// input together with (GLen-1) 1-bit shift registers. Thus
	// decoderTableSize should be 2^(GLen+1).
	//
	lookupTable = new struct DecoderLookupEntry[decoderTableSize];

	for (int i = 0; i < decoderTableSize; i++) {
		lookupTable[i].nextState = -1;
	}

	for (int i = 0, j; i < encoderTableSize; i++) {
		j = i & ((1 << (GLen - 1)) - 1);
		x = encoderLookupTable[i].x << GLen;
		y = encoderLookupTable[i].y << (GLen - 1);
		j = j | x | y;

		lookupTable[j].out = (i >> (GLen - 1));
		lookupTable[j].nextState = encoderLookupTable[i].nextState;
	}

	return lookupTable;
}

void ConvoCode_11a::
setEncoderLookupTable(struct EncoderLookupEntry *lookupTable)
{
	encoderLookupTable = lookupTable;
}

void ConvoCode_11a::
setDecoderLookupTable(struct DecoderLookupEntry *lookupTable)
{
	decoderLookupTable = lookupTable;
}

void ConvoCode_11a::resetEncoderState()
{
	encoderState = 0;
}

void ConvoCode_11a::resetDecoderState()
{
	int size = (int) pow(2, GLen - 1);	// Number of states

	decoderState[0] = 0;
	for (int i = 1; i < size; i++) {
		decoderState[i] = -1;
	}
}

/*
 * Convolutional encoder
 */
int ConvoCode_11a::encode(char *input, char *output, int inputLen)
{
	int in;
	int n = 0;		// Number of output bits
	int shift;

	for (int i = 0; i < inputLen; i++) {
		in = (input[i / 8] << (i % 8));
		in = (in & 0x80) >> (8 - GLen);
		in = in | encoderState;
		encoderState = encoderLookupTable[in].nextState;

		shift = PLen - (i % PLen) - 1;

		if ((PX >> shift) & 0x1) {
			// Take X
			output[n / 8] <<= 1;
			output[n / 8] |= encoderLookupTable[in].x;
			n++;
		}

		if ((PY >> shift) & 0x1) {
			// Take Y
			output[n / 8] <<= 1;
			output[n / 8] |= encoderLookupTable[in].y;
			n++;
		}
	}

	return n;
}

/*
 * Convolutional decoder (Viterbi algorithm)
 */
int ConvoCode_11a::decode(char *input, char *output, int inputLen,
		      int lookahead)
{
	int **from;
	int **metric;
	int size = (int) pow(2, GLen - 1);	// Number of states
	int nbits =
	    (inputLen + lookahead) * PLen / (BITCOUNT(PX) +
					     BITCOUNT(PY)) * 2;
	int state, nextState, prevState;
	int i, j;

	from = new int *[size];
	metric = new int *[size];
	for (i = 0; i < size; i++) {
		from[i] = new int[nbits / 2 + 1];
		metric[i] = new int[nbits / 2 + 1];

		for (j = 0; j < nbits / 2 + 1; j++) {
			from[i][j] = -1;
			metric[i][j] = -1;
		}
	}

	// Initial state
	for (i = 0; i < size; i++) {
		metric[i][0] = decoderState[i];
	}

	// Construct the trellis diagram
	int x, y, shift;
	int consumed = 0;
	for (i = 0; consumed < inputLen + lookahead; i++) {
		// Extract 2-bit input
		shift = PLen - (i % PLen) - 1;
		x = -1;
		y = -1;

		if ((PX >> shift) & 0x1) {
			x = input[consumed / 8] << (consumed % 8);
			x = (x & 0x80) >> 7;
			consumed++;
		}

		if ((PY >> shift) & 0x1) {
			y = input[consumed / 8] << (consumed % 8);
			y = (y & 0x80) >> 7;
			consumed++;
		}

		for (j = 0; j < size; j++) {
			if (metric[j][i] != -1) {
				for (int m = 0; m < 4; m++) {
					// m is one of 00, 01, 10, 11
					state = (m << (GLen - 1)) | j;
					nextState =
					    decoderLookupTable[state].
					    nextState;

					if (nextState != -1) {
						// Calculate Hamming distance metric
						int tmp = metric[j][i];

						if (x != -1
						    && x == (m >> 1)) {
							tmp++;
						}

						if (y != -1
						    && y == (m & 0x1)) {
							tmp++;
						}

						if (tmp >
						    metric[nextState][i +
								      1]) {
							from[nextState][i +
									1]
							    = j;
							metric[nextState][i
									  +
									  1]
							    = tmp;
						}
					}
				}
			}
		}
	}
/*
	for (i=0; i<size; i++)
	{
		printf("%3d: ", i);
		for (j=0; j<nbits/2+1; j++)
		{
			if (metric[i][j] == -1)
			{
				printf("     X-");
			}
			else
			{
				printf("%2d %3d-", from[i][j], metric[i][j]);
			}
		}
		printf("\n");
	}
*/

	// Find the path with the largest metric
	int max = 0;
	for (i = 0; i < size; i++) {
		if (metric[i][nbits / 2] >= max) {
			max = metric[i][nbits / 2];
			state = i;
		}
	}

	// Backtrack
	j = inputLen * PLen / (BITCOUNT(PX) + BITCOUNT(PY)) * 2;
	for (i = nbits / 2; i > j; i--) {
		state = from[state][i];
	}

	for (; i > 0; i--) {
		prevState = from[state][i];
		output[(i - 1) / 8] >>= 1;

		if (prevState == 0 && state == 0) {
			output[(i - 1) / 8] &= 0x7F;
		} else if (prevState == (size - 1) && state == (size - 1)) {
			output[(i - 1) / 8] |= 0x80;
		} else if (prevState >= state) {
			// Going upward in the trellis diagram means a 0
			output[(i - 1) / 8] &= 0x7F;
		} else {
			// Going downward in the trellis diagram means a 1
			output[(i - 1) / 8] |= 0x80;
		}
		state = prevState;
	}

	// Record decoder state and cleanup
	nbits = inputLen * PLen / (BITCOUNT(PX) + BITCOUNT(PY)) * 2;

	j = metric[0][0];
	for (i = 0; i < size; i++) {
		decoderState[i] = metric[i][nbits / 2] - j;

		delete from[i];
		delete metric[i];
	}
	delete from;
	delete metric;

	return nbits / 2;
}
