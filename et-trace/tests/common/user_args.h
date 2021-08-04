#ifndef USER_ARGS_H
#define USER_ARGS_H

#ifdef __cplusplus
extern "C" {
#endif

struct user_args {
    int seed;
    const char *output;
};

void parse_args(int argc, const char **argv, struct user_args *uargs);

#ifdef __cplusplus
}
#endif

#endif
