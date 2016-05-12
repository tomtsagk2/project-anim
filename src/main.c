#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include "asarray.h"
#include "dy_array.h"

//I don't like these keys, probably I will change it

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

/* Custom drawing
 * Describes a command that will execute for some frames.
 * The command should ideally affect the resulting frame by
 * drawing on it, appending images on it, or distorting it in
 * some other way.
 */
char *custom_keys[] = {
	"custom",
	"from",
	"to",
	(char*) 0,
};

//Variables

//Structs and arrays
struct AsArray template;
struct AsArray tools;
struct dy_array primitives;

/* Custom sentence with arguments
 * contains dynamic arrays with letters.
 * each dynamic array is one word (argument)
 */
struct dy_array2 cus_sen;

/* Runtime variables
 * These values are used for runtime things, like how many
 * frames to draw, which file to parse etc etc
 * render_from can be from 0 to duration-1
 * render_to can be from render_from to duration-1
 * if render_to is -1, it will change to duration-1 and
 * draw the whole animation.
 */
char *input_file;
int render_from;
int render_to;

/* Global Settings
 * Used for settings that change each time the program runs. For example
 * showing output with -verbose`
 */
char verbose;

//Functions
int exec(const char* c, char *const argv[]);
void init_defaults();
void parse_args(int argc, char *argv[]);
int parse_file();
void render_frames();
void render_output();
void clean();

int main (int argc, char *argv[]) {

	/* Organized steps */

	//Init variables to default
	init_defaults();

	//Parse arguments (argv)
	parse_args(argc, argv);

	//Parse input file
	if (parse_file() < 0) {
		printf("terminating due to fatal error\n");
		return -1;
	}

	//Render frames
	render_frames();

	//Combine frames to animation
	render_output();

	//Clean allocated data
	clean();

	//Finish OK
	if (verbose != 's') {
		printf("done\n");
	}
	return 0;
}


/* Forks child and waits for it to execute a
 * program with arguments. Then returns its
 * return status.
 */
int exec(const char* c, char *const argv[]) {
	//New process id
	pid_t pID = fork();

	//Handle fork error
	if (pID < 0) {
		printf("Fork error!\n");
		return -1;
	} else
	//Child's execution
	if (pID == 0) {

		//Print command before executing it
		if (verbose == 'v') {
			int i = 0;
			while (argv[i] != (char*) 0) {
				printf("%s ", argv[i]);
				i++;
			}
			printf("\n");
		}

		//Execute program with args
		if (execvp(c, argv) == -1) {
			printf("child error on exec: %s\n", c);
			exit(-1);
		}

		//Code shouldn't reach this, but just to be sure
		exit(0);
	}

	//Parent waits for child and returns
	int returner;
	wait(&returner);
	return returner;
}

void init_defaults() {
	//Template
	init_ar(&template, template_keys);
	ar_set(&template, "width" , "10");
	ar_set(&template, "height", "10");
	ar_set(&template, "duration", "10");
	ar_set(&template, "output", "final.gif");

	//Tools
	init_ar(&tools, tool_keys);
	ar_set(&tools, "fill", "#cc99ff");
	ar_set(&tools, "background", "#222222");

	//Primitives
	dr_init(&primitives);

	/* Custom sentence with arguments
	 * contains dynamic arrays with letters.
	 * each dynamic array is one word (argument)
	 */
	dr_init2(&cus_sen, sizeof(struct dy_array2));

	/* Runtime variables
	 * These values are used for runtime things, like how many
	 * frames to draw, which file to parse etc etc
	 * render_from can be from 0 to duration-1
	 * render_to can be from render_from to duration-1
	 * if render_to is -1, it will change to duration-1 and
	 * draw the whole animation.
	 */
	input_file = 0;
	render_from = 0;
	render_to   = -1;

	verbose = 'n';
}

void parse_args(int argc, char*argv[]) {
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
			verbose = 'v';
		} else
		if ( strcmp(argv[i], "-s") == 0) {
			verbose = 's';
		}
	}
}

int parse_file() {
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
		if ( strcmp(buffer, primitive_keys[0]) == 0 ) {
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
		} else
		//Custom elements
		if ( strcmp(buffer, custom_keys[0]) == 0) {
			//Read word
			int res = fscanf(f, "%*[ \t]%[^ \t\n]", buffer);

			//While words exist
			while (res != EOF
			&& res != 0
			&& strcmp(buffer, "end") != 0) {
				//Check if word has quotes
				if (buffer[0] == '"' || buffer[0] == '\'') {
					char temp[256];
					fscanf(f, "%[^\"]%*1c", temp);
					strcat(buffer, temp);
					//printf("Argument:%s\n", buffer+1);

					//Create new dynamic array for word(argument)
					struct dy_array2 cus_word;
					dr_init2_fixed(&cus_word, sizeof(char), strlen(buffer+1));

					//Apply word from buffer to dynamic array
					dr_add2_size(&cus_word, buffer+1, strlen(buffer+1));

					//Add word to sentence
					dr_add2(&cus_sen, &cus_word);

					//DEBUG - print word using sentence
					/*
					printf("Argument: %s\n", 
						(char*) ((struct dy_array2*) cus_sen.ar)
							[cus_sen.elements-1].ar);
					*/
				}
				//Word has no quotes, apply to dynamic array
				else {
					//Create new dynamic array for word(argument)
					struct dy_array2 cus_word;
					dr_init2_fixed(&cus_word, sizeof(char), strlen(buffer));

					//Apply word from buffer to dynamic array
					dr_add2_size(&cus_word, buffer, strlen(buffer));

					//Add word to sentence
					dr_add2(&cus_sen, &cus_word);

					//DEBUG - print word using sentence
					/*
					printf("Argument: %s\n", 
						(char*) ((struct dy_array2*) cus_sen.ar)
							[cus_sen.elements-1].ar);
					*/
				}

				//Next word (stop at new line)
				res = fscanf(f, "%*[ \t]%[^ \t\n]", buffer);
			}

			//Ignore rest for now
			fscanf(f, "%*[^end]%*3c");

			/*
			//Create temp custom
			struct AsArray cust;
			init_ar(&cust, custom_keys);

			//Get key
			char* key = malloc( sizeof(char) *strlen(buffer));
			strcpy(key, buffer);

			//Get value
			fscanf(f, "%*[ \t\n]%[^\n]%*1c", buffer);
			char* val = malloc( sizeof(char) *strlen(buffer));
			strcpy(val, buffer);

			//Apply value to key
			ar_set(&cust, key, val);

			//Scan custom values
			while (fscanf(f, "%s", buffer) != EOF 
			&& strcmp(buffer, "end") != 0) {
				//Word inside custom's values
				if (ar_exists(&cust, buffer)) {
					//Get key
					char* key = malloc( sizeof(char) *strlen(buffer));
					strcpy(key, buffer);

					//Get value
					fscanf(f, "%*[ \t\n]%[^\n]%*1c", buffer);
					char* val = malloc( sizeof(char) *strlen(buffer));
					strcpy(val, buffer);
	
					//Apply value to key
					ar_set(&cust, key, val);
				}
				//Word not a primitive value
				else {
					printf("parsing error: '%s' is not a custom value\n", buffer);
					return -1;
				}
			}

			//Check if custom is inside rendering range
			if ( ar_get_int(&cust, "from") > render_to
			||   ar_get_int(&cust, "to"  ) < render_from) {
				printf("one custom is skipped\n");
			}
			else {
				dr_add(&customs, cust);
			}
			*/
		}
		else {
			printf("Ignored: %s\n", buffer);
		}
	}

	//Close file
	fclose(f);

	return 0;

	//Handle parsing error
	error:

	if (ferror(f)) {
		fprintf(stderr, "error while parsing: %s: %s\n", input_file,
			strerror(errno));
	} else
	if (feof(f)) {
		printf("unexpected end of file: %s\n", input_file);
	}
	else {
		printf("parsing error: %s\n", input_file);
	}

	printf("maybe it has something to do with '%s'?\n", buffer);

	//Close file
	if (f) {
		fclose(f);
	}

	//Return error
	return -1;
}

void render_frames() {
	//Start drawing data
	char size[100];
	sprintf(size, "%sx%s", ar_get(&template, "width"), 
		ar_get(&template, "height"));
	for (int i = render_from; i <= render_to; i++) {
		//Create image
		char out_frame[100];
		sprintf(out_frame, "rendered/frame_%04d.png", i);
		if (verbose != 's') {
			printf("Rendering frame: %s\n", out_frame);
		}
	
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

		//Draw customs in frame

		char **args2 = malloc( sizeof(char*) *cus_sen.elements +1);
		
		//For each word in sentence
		for (unsigned int z = 0; z < cus_sen.elements; z++) {
			if ( strcmp( ((char*) ((struct dy_array2*)cus_sen.ar)[z].ar),
			"$frame") == 0 ) {
				//printf("found a frame!\n");
				//printf("%s has %d letters\n", out_frame, strlen(out_frame));
				args2[z] = malloc( sizeof(char) *(strlen(out_frame)+1));
				args2[z][0] = '\0';
				strcat(args2[z], out_frame);
			}
			else {
				args2[z] = ((struct dy_array2*) cus_sen.ar)[z].ar;
			}
		}

		//Add null pointer on last argument
		args2[cus_sen.elements] = (char*) 0;

		if ( exec("convert", args2) != 0 ) {
			printf("custom error\n");
			exit(-1);
		}
		/*
		for (unsigned int z = 0; z < customs.elements; z++) {
			//Check if customs are visible
			if (i >= ar_get_int(&customs.ar[z], "from") 
			&&  i <= ar_get_int(&customs.ar[z], "to"  )) {
				;
			}
		}
		*/
	}
}

void render_output() {
	//Link frames together
	if (verbose != 's') {
		printf("converting to animation '%s' ...\n", ar_get(&template, "output"));
	}
	char *args[] = {"convert", "rendered/*.png", 
		ar_get(&template, "output"), 
		(char*) 0};
	exec("convert", args);
}

void clean() {

}
