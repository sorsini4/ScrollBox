/* scroll.c -- scrolling display
 *
 * Darren Provine, 20 March 2012
 * Modified by S Orsini 8 Dec 2020
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

// sleep needs this
#include <unistd.h>

#include "scroll.h"
#define Size 50

// debug flag - changed via -D option
int debug = 0;
char time_string[Size];
char date_string[Size];
char week_string[Size];
char test_string[Size];
char military_string[Size];

static char bugaddress[]="kilroy@elvis.rowan.edu";

// version -- say which version this is and exit
// (note simple output; you could also print the rcsid too)
void version()
{
    fprintf(stderr, "scroll version 1\n");
    exit(0);
}

// usage -- print brief summary and instructions
void usage(char *progname)
{
    fprintf(stderr, "This program shows a scrolling box.\n");
    fprintf(stderr, "Usage: %s [-dDvh] text\n", progname);
    fprintf(stderr,
            "  -d # : set scroll delay to # milliseconds (default=1000)\n");
    fprintf(stderr,
            "  -D   : turn on debug messages (more Ds = more debugging)\n");
    fprintf(stderr, "  -v   : show version information\n");
    fprintf(stderr, "  -h   : this help message\n");
    fprintf(stderr, "Use quotes for multi-word messages, 'like this'.\n");
    fprintf(stderr, "report bugs to %s \n", bugaddress);
    exit (0);
}

void exit_scroll()
{
    end_display();
    exit(0);
}

// keyboard and mouse handling
void process_key(keybits KeyCode)
{
    int row, col;
    if ( KeyCode & 0x80 ) { // mouse click
        row = (KeyCode & 0x70) >> 4;
        col = (KeyCode & 0x0f);

        if(row == 0 && col == 0) {
            speed_up();
        }
        else if(row == 0 && col == 1) {
            slow_down();
        }
        else if(row == 0 && col == 2) {
            update_time();
        }
        else if(row == 0 && col == 3) {
            test_mode();
        }
        else if(row == 0 && col == 4) {
            exit_scroll();
        }
        else if(row == 1 && col == 0) {
            date_mode();
        }   
        else if(row == 1 && col == 1) {
            week_mode();
        }
        else if(row == 1 && col == 2) {
            military_mode();
        }
    } else { // keyboard press
        switch(KeyCode) {
            case 'f': speed_up();     break;
            case 's': slow_down();    break;
            case 'i': update_time();  break;
            case 't': test_mode();    break;
            case 'q': exit_scroll();  break;
            case 'd': date_mode();    break;
            case 'w': week_mode();    break;
            case 'm': military_mode(); break;
        }
    }
}

void speed_up() {
    if( (get_delay() - 300) > 100) {
        start_timer( (get_delay() - 300));
    }
}

void slow_down() {
    if( (get_delay() + 300) < 5000) {
        start_timer( (get_delay() + 300) );
    }
}

void update_time() {
    //getting the current delay to change back too
    int curr_delay = get_delay();
    //getting the time struct
    time_t curr = time(NULL);
    struct tm *tmp = localtime(&curr);
    strftime(time_string, 256, "%-I:%M:%S%P", tmp);
    show(time_string);
    //starting the time struct for 5 seconds, and sleeping for 5 seconds within it
    start_timer(5000);
    sleep(5000);
    //going back to the original delay
    start_timer(curr_delay);
}

void test_mode() {
    int curr_delay = get_delay();
    strncpy(test_string, "\n\n\n\n\n\n\n\n\n\n\n", 11); 
    show(test_string);
    start_timer(5000);
    sleep(5000);
    start_timer(curr_delay);
}

void date_mode() {
    int curr_delay = get_delay();
    time_t curr = time(NULL);
    struct tm *tmp = localtime(&curr);
    strftime(date_string, 256, "%D %a", tmp);
    show(date_string);
    start_timer(5000);
    sleep(5000);
    start_timer(curr_delay);
}

void week_mode() {
    int curr_delay = get_delay();
    time_t curr = time(NULL);
    struct tm *tmp = localtime(&curr);
    strftime(week_string, 256, "%a, %b %d", tmp);
    show(week_string);
    start_timer(5000);
    sleep(5000);
    start_timer(curr_delay);
}

void military_mode() {
    int curr_delay = get_delay();
    time_t curr = time(NULL);
    struct tm *tmp = localtime(&curr);
    strftime(military_string, 256, "%k:%M:%S  ", tmp);
    show(military_string);
    start_timer(5000);
    sleep(5000);
    start_timer(curr_delay);
}

int main(int argc, char *argv[])
{
    int   initial_delay = 1000;
    int   letter; // option char, if any
    // note C does automatic concatenation of long strings
    char  title[81] =
               "------------------------------  "
               "Stevens Scroll Box"
               "  -----------------------------";

    // loop through all the options; getopt() can handle together or apart
    while ( ( letter = getopt(argc, argv, "d:Dvh")) != -1 ) {
        switch (letter) {
            case 'd':  initial_delay = atoi(optarg); break;
            case 'D':  debug++;                      break;
            case 'v':  version();                    break;
            case 'h':  usage(argv[0]);               break;

            case '?':  // unknown flag; fall through to next case
            default:   // shouldn't happen, but Just In Case
                       // note that getopt() warns about the unknown flag
                  fprintf(stderr, "run \"%s -h\" for help\n", argv[0]);
                  exit(1);
        }
    }

    // optind is the first argument after options are processed
    // if there aren't any, the user didn't give a message to scroll
    if (optind == argc) {
        usage(argv[0]);
    }

    // put the information from the command line into the module
    setup(argv[optind]);

    // set up the view
    register_keyhandler(process_key);
    start_display();
    set_title_bar(title);

    //set up buttons
    set_key_text(0, "Date");
    set_key_text(1, "Week");
    set_key_text(2, "24 Hr");

    // start the model running
    start_timer(initial_delay);

    // wait for the model to signal controller
    while ( 1 ) {
        get_key();
    }

    end_display();

    return 0;
}


/* This is the function the model uses to signal the controller
 * that there's work to do.
 */
void tick(int sig)
{
    char   *segment;

    /* get the information from model about what chars to show
     * and how far over to slide them */
    segment = display_string();
    
    if (debug >= 3) {
        fprintf(stderr, "display_string() returned |%s|\r\n", segment);
        sleep(2);
    }
    /* Send those chars to the view. */
    show(segment);
    get_key();
}

