//
// ANSI escape sequences used to change font colors
//


#define ESC			"\033"
#define ESC_CHAR	'\033'
 
// Puts everything back to normal

#define ANSI_NORMAL ESC "[2;37;0m"     

// Non-color based font changes
 
#define ANSI_BOLD 		ESC "[1m"	/* Turn on bold mode */
#define ANSI_BLINK 		ESC "[5m"	/* Initialize blink mode */
#define ANSI_UNDERLINE 		ESC "[4m"	/* Initialize underscore mode */
#define ANSI_REVERSE 		ESC "[7m"	/* Turns reverse video mode on */
#define ANSI_HIGH_REVERSE 	ESC "[1,7m"	/* Hi intensity reverse video  */
 
//  Foreground Colors  
 
#define ANSI_BLACK 	ESC "[30m"
#define ANSI_RED 	ESC "[31m"
#define ANSI_GREEN 	ESC "[32m"
#define ANSI_YELLOW 	ESC "[33m"
#define ANSI_BLUE 	ESC "[34m"
#define ANSI_MAGENTA 	ESC "[35m"
#define ANSI_CYAN 	ESC "[36m"
#define ANSI_WHITE 	ESC "[37m"
 
//  Hi Intensity Foreground Colors  
 
#define ANSI_HIGH_RED 		ESC "[1;31m"	
#define ANSI_HIGH_GREEN 	ESC "[1;32m"	
#define ANSI_HIGH_YELLOW 	ESC "[1;33m"	
#define ANSI_HIGH_BLUE 		ESC "[1;34m"	
#define ANSI_HIGH_MAGENTA 	ESC "[1;35m"	
#define ANSI_HIGH_CYAN 		ESC "[1;36m"	
#define ANSI_HIGH_WHITE 	ESC "[1;37m"	
 
// Background Colors
 
#define ANSI_BACKGROUND_BLACK 	ESC "[40m"
#define ANSI_BACKGROUND_RED 	ESC "[41m"
#define ANSI_BACKGROUND_GREEN 	ESC "[42m"
#define ANSI_BACKGROUND_YELLOW 	ESC "[43m"
#define ANSI_BACKGROUND_BLUE 	ESC "[44m"
#define ANSI_BACKGROUND_MAGENTA ESC "[45m"
#define ANSI_BACKGROUND_CYAN 	ESC "[46m"
#define ANSI_BACKGROUND_WHITE 	ESC "[47m"
 
// High Intensity Background Colors 
 
#define ANSI_HIGH_BACKGROUND_RED 	ESC "[41;1m"
#define ANSI_HIGH_BACKGROUND_GREEN 	ESC "[42;1m"
#define ANSI_HIGH_BACKGROUND_YELLOW 	ESC "[43;1m"
#define ANSI_HIGH_BACKGROUND_BLUE 	ESC "[44;1m"
#define ANSI_HIGH_BACKGROUND_MAGENTA 	ESC "[45;1m"
#define ANSI_HIGH_BACKGROUND_CYAN 	ESC "[46;1m"
#define ANSI_HIGH_BACKGROUND_WHITE 	ESC "[47;1m"
 
