#include "utils.h"

// void handleFlags(int argc, char** argv, unsigned int* height, char** dataFileName, char** pattern, char* skewFlag) {
//     if (argc == 7 || argc == 8) {
//         if (strcmp(argv[1], "-h") == 0) {
//             int heightArg = atoi(argv[2]);
//             if (heightArg < 1 || heightArg > 5) {
//                 printf("Invalid height parameter\nExiting...\n");
//                 exit(1);
//             }

//             (*height) = (unsigned int)heightArg;
//         } else {
//             printf("Invalid flags\nExiting...\n");
//             exit(1);
//         }

//         if (strcmp(argv[3], "-d") == 0) {
//             (*dataFileName) = argv[4];
//         } else {
//             printf("Invalid flags\nExiting...\n");
//             exit(1);
//         }

//         if (strcmp(argv[5], "-p") == 0) {
//             (*pattern) = argv[6];
//         } else {
//             printf("Invalid flags\nExiting...\n");
//             exit(1);
//         }

//         if (argc == 8) {
//             if (strcmp(argv[7], "-s") == 0) {
//                 (*skewFlag) = 1;
//             } else {
//                 printf("Invalid flags\nExiting...\n");
//                 exit(1);
//             }
//         } else
//             (*skewFlag) = 0;
//     } else {
//         printf("Invalid flags\nExiting...\n");
//         exit(1);
//     }
// }