/* bin2cinecutie_header.c
* 
* 2007 March
* modified by Yaco <yaco-at-gnu.org>
 * v1.2 - fixed file head and tail
 *      - now output is ready to use in cinecutie
 *	- added a bash script to quicly convert all plugins icons
 *
 * 2005 July
 * modified for cinecutie-cvs by Dom <binary1230-at-yahoo.com>
 * v1.1 - renamed from bin2c.c to bin2cinecutie_header.c for cinecutie use
 *      - adds 4 extra bytes for original filesize for use with cinecutie CVS.
 *
 * input:
 *   any binary file
 *
 * output:
 *   A C header file containing an unsigned char array with the following properties:
 *   -- A 4 byte value indicating the length of the binary data
 *   -- The data itself
 *
 * this utility is useful for modifying the logos in cinecutie which are stored as arrays
 * of binary data.  
 * 
 * original bin2c (c)2000 Dan Potter, BSD license.
*/

#include <stdio.h>

void convert(char *ifn, char *ofn, char *prefix) {
	FILE *i, *o;
	unsigned char buffer[2048];
	int red, left, lc, q;
	char buf[BUFSIZ];

	i = fopen(ifn, "rb");
	o = fopen(ofn, "w");
	if (!i || !o) {
		printf("error: can't open input or output file\n");
		return;
	}

	fseek(i, 0, SEEK_END); left = ftell(i); fseek(i, 0, SEEK_SET);
	setbuf(o, buf);

	fprintf(o, "#ifndef PICON_PNG_H\n"
		   "#define PICON_PNG_H\n");
	
	// NOTE: not used for cinecutie
	// fprintf(o, "const int %s_size = %d\n", prefix, left);

	fprintf(o, "static unsigned char picon_png[]=");
	fprintf(o, "{\n\t");

	// print the size 
	fprintf(o, "0x%02x, ", (left >> 24) & 0xFF);
	fprintf(o, "0x%02x, ", (left >> 16) & 0xFF);
	fprintf(o, "0x%02x, ", (left >> 8) & 0xFF);
	fprintf(o, "0x%02x, ", (left >> 0) & 0xFF);

	fprintf(o, "\n\t");

	// print the data
	lc = 0;
	while(left > 0) {
		red = fread(buffer, 1, 2048, i);
		left -= red;
		for (q=0; q<red; q++) {
			fprintf(o, "0x%02x, ", buffer[q]);
			if ((++lc) >= 8) {
				lc = 0;
				fprintf(o, "\n\t");
			}
		}
	}

	fprintf(o, "\n};\n");
	fprintf(o, "#endif\n");
	fclose(i); fclose(o);
}

int main(int argc, char **argv) {

	char *prefix;

	argc--;
	if (argc != 2 && argc != 3) {
		printf("usage: bin2cinecutie_header <input> <output> [prefix]\n");
		return 0;
	}

	prefix = (argc == 3) ? argv[3] : "file";
	convert(argv[1], argv[2], prefix);
	return 0;
}

