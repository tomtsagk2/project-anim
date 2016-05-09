#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include "asarray.h"
#include "dy_array.h"

/* Template keys
 * Shows what data are included inside the animation's "template".
 * Template data, are data that are assigned once (from a header) and
 * remain the same throught the animation, like animation's width.
 */
char *template_keys[] = {
	"width", 
	"height", 
	"duration", 
	"output", 
	(char*) 0,
};

/* Tool keys
 * Shows what tools are available. Tools can only be changed
 * after the header, and between drawing objects.
 */
char *tool_keys[] = {
 	"fill",
	"background",
	(char*) 0,
};

/* Primitive keys
 * Describe a primitive that is visible to the screen.
 * First variable, is used to tell apart primitives from other
 * elements (like images).
 */
char *primitive_keys[] = {
	"primitive",
	"from",
	"to",
	(char*) 0,
};

//Functions
int exec(const char* c, char *const args[]);

/* Global Settings
 * Used for settings that change each time the program runs. For example
 * showing output with -verbose`
 */
char verbose = 0;

int main (int argc, char *argv[]) {
	
	//Not enough arguments
	/*
	if (argc < 2) {
		printf("not enough arguments\n");
		printf("usage: program -i input_file\n");
		return -1;
	}
	*/

	//Template
	struct AsArray template;
	init_ar(&template, template_keys);

	//Default template
	ar_set(&template, "width" , "10");
	ar_set(&template, "height", "10");
	ar_set(&template, "duration", "10");
	ar_set(&template, "output", "final3.gif");

	//Tools
	struct AsArray tools;
	init_ar(&tools, tool_keys);

	//Default tools
	ar_set(&tools, "fill", "#cc99ff");
	ar_set(&tools, "background", "#222222");

	//Primitives
	struct dy_array primitives;
	dr_init(&primitives);

	/* Runtime variables
	 * These values are used for runtime things, like how many
	 * frames to draw, which file to parse etc etc
	 * render_from can be from 0 to duration-1
	 * render_to can be from render_from to duration-1
	 * if render_to is -1, it will change to duration-1 and
	 * draw the whole animation.
	 */
	char *input_file = 0;
	int render_from = 0;
	int render_to   = -1;

	//Check arguments
	for (int i = 0; i < argc; i++) {
		if ( strcmp(argv[i], "-i") == 0 ) {
			input_file = argv[i+1];
			i++;
		} else
		if ( strcmp(argv[i], "--render-from") == 0) {
			render_from = atoi(argv[i+1]);
			i++;
		} else
		if ( strcmp(argv[i], "--render-to") == 0) {
			render_to = atoi(argv[i+1]);
			i++;
		} else
		if ( strcmp(argv[i], "--verbose") == 0) {
			verbose = 'y';
		}
	}

	//Open file
	FILE *f;
	if (input_file)
		f = fopen(input_file, "r");
	else {
		printf("error: no input file\n");
		printf("usage: program -i input_file\n");
		return -1;
	}

	//Check errors
	if (!f) {
		printf("error %s: %s\n", input_file,
			strerror(errno));
		return 0;
	}

	/* Header
	 * scan word by word until end_header
	 */
	char buffer[256];
	while (fscanf(f, "%s", buffer) != EOF
	&& strcmp(buffer, "end_header") != 0) {
		//Ignore comments
		if (buffer[0] == '#') {
			if (fscanf(f, "%*[^\n]%*1c") == EOF) {
				goto error;
			}
		} else
		//Word is inside template
		if ( ar_exists(&template, buffer) ) {
			//Get key
			char* key = malloc( sizeof(char) *strlen(buffer));
			strcpy(key, buffer);

			//Get value
			fscanf(f, "%*[ \t\n]%[^\n]%*1c", buffer);
			char* val = malloc( sizeof(char) *strlen(buffer));
			strcpy(val, buffer);

			//Apply value to key
			ar_set(&template, key, val);
		}
		//Word not in template
		else {
			printf("unexpected element: %s\n", buffer);
			goto error;
		}
	}
	//Header ended

	//Validate template's values
	int duration = ar_get_int(&template, "duration");
	if (render_to == -1) render_to = duration-1;

	if (render_from < 0 || render_from >= duration
	||  render_to < render_from
	||  render_to >= duration) {
		printf("error: cannot draw from %d to %d\n", render_from, render_to);
		printf("the animation's duration is from 0 to %d\n",
			duration-1);
		return -1;
	}

	//Parse the rest of the file
	while (fscanf(f, "%s", buffer) != EOF) {
		//Ignore comments
		if ( buffer[0] == '#' ) {
			fscanf(f, "%*[^\n]%*1c");
		} else
		//Tools
		if ( ar_exists(&tools, buffer) ) {
			//Get key
			char* key = malloc( sizeof(char) *strlen(buffer));
			strcpy(key, buffer);

			//Get value
			fscanf(f, "%*[ \t\n]%[^\n]%*1c", buffer);
			char* val = malloc( sizeof(char) *strlen(buffer));
			strcpy(val, buffer);

			//Apply value to key
			ar_set(&tools, key, val);
		} else
		//Primitive
		if ( strcmp(buffer, "primitive") == 0) {
			//Create temp primitive
			struct AsArray prim;
			init_ar(&prim, primitive_keys);

			//Get key
			char* key = malloc( sizeof(char) *strlen(buffer));
			strcpy(key, buffer);

			//Get value
			fscanf(f, "%*[ \t\n]%[^\n]%*1c", buffer);
			char* val = malloc( sizeof(char) *strlen(buffer));
			strcpy(val, buffer);

			//Apply value to key
			ar_set(&prim, key, val);

			//Scan primitive values
			while (fscanf(f, "%s", buffer) != EOF 
			&& strcmp(buffer, "draw_primitive") != 0) {
				//Word inside primitive's values
				if (ar_exists(&prim, buffer)) {
					//Get key
					char* key = malloc( sizeof(char) *strlen(buffer));
					strcpy(key, buffer);

					//Get value
					fscanf(f, "%*[ \t\n]%[^\n]%*1c", buffer);
					char* val = malloc( sizeof(char) *strlen(buffer));
					strcpy(val, buffer);
	
					//Apply value to key
					ar_set(&prim, key, val);
				}
				//Word not a primitive value
				else {
					printf("parsing error: '%s' is not a primitive value\n", buffer);
					return -1;
				}
			}

			//Check if primitive is inside rendering range
			if ( ar_get_int(&prim, "from") > render_to
			||   ar_get_int(&prim, "to"  ) < render_from) {
				printf("one primitive is skipped\n");
			}
			else {
				dr_add(&primitives, prim);
			}
		}
		else {
			printf("Ignored: %s\n", buffer);
		}
	}

	//Close file
	fclose(f);

	//Start drawing data
	char size[100];
	sprintf(size, "%sx%s", ar_get(&template, "width"), 
		ar_get(&template, "height"));
	for (int i = render_from; i <= render_to; i++) {
		//Create image
		char out_frame[100];
		sprintf(out_frame, "rendered/frame_%04d.png", i);
		printf("Rendering frame: %s\n", out_frame);
	
		//Create empty frame
		char *bg = malloc( sizeof(char) *(strlen(ar_get(&tools, "background") +3)) );
		bg[0] = 'x';
		bg[1] = 'c';
		bg[2] = ':';
		bg[3] = '\0';
		strcat(bg, ar_get(&tools, "background"));
		char *args_temp[] = {"convert", "-size", size, bg,
			out_frame, (char*)0};
		exec("convert", args_temp);

		//Draw primitives on frame
		for (unsigned int z = 0; z < primitives.elements; z++) {
			//Check if primitives are visible
			if (i >= ar_get_int(&primitives.ar[z], "from") 
			&&  i <= ar_get_int(&primitives.ar[z], "to"  )) {
				char *args[] = {"convert", out_frame,
					"-fill", ar_get(&tools, "fill"),
					"-draw", ar_get(&primitives.ar[z], "primitive"), 
					out_frame, (char*)0};
				exec("convert", args);
			}
		}
	}
	
	//Link frames together
	printf("converting to animation '%s' ...\n", ar_get(&template, "output"));
	char *args[] = {"convert", "rendered/*.png", 
		ar_get(&template, "output"), 
		(char*) 0};
	exec("convert", args);

	//Finish OK
	printf("done\n");
	return 0;

	//Handle parsing error
	error:

	if (ferror(f)) {
		fprintf(stderr, "error while parsing: %s: %s\n", argv[1],
			strerror(errno));
	} else
	if (feof(f)) {
		printf("unexpected end of file: %s\n", argv[1]);
	}
	else {
		printf("parsing error: %s\n", argv[1]);
	}

	printf("maybe it has something to do with '%s'?\n", buffer);

	//Close file
	if (f) {
		fclose(f);
	}

	//Return error
	return -1;
}


/* Forks child and waits for it to execute a
 * program with arguments. Then returns its
 * return status.
 */
int exec(const char* c, char *const args[]) {
	//New process id
	pid_t pID = fork();

	//Handle fork error
	if (pID < 0) {
		printf("Fork error!\n");
		return -1;
	} else
	//Child's execution
	if (pID == 0) {

		//Print commant before executing it
		if (verbose) {
			char buffer[256];
			buffer[0] = '\0';
			int i = 0;
			while (args[i] != (char*) 0) {
				strcat(buffer, args[i]);
				strcat(buffer, " ");
				i++;
			}
			printf("%s\n", buffer);
		}

		//Execute program with args
		if (execvp(c, args) == -1) {
			printf("child error on exec: %s\n", c);
			return -1;
		}

		//Code shouldn't reach this, but just to be sure
		return 0;
	}

	//Parent waits for child and returns
	int returner;
	wait(&returner);
	return returner;
}
