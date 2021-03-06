#include <stdlib.h>
#include <stdio.h>

#include "object.h"
#include "list.h"
#include "hash.h"
#include "reader.h"
#include "env.h"
#include "eval.h"
#include "error.h"

struct Envir* global_env;
struct Envir* user_env;
extern struct Envir* curr_env;

struct Object* true_obj;
struct Object* nil_obj;

struct Object* lat_cons(struct List* args) {
    if(list_bare_length(args) != 2) {
        printf("error: cons expects two arguments\n");
        return error_init(arg_err, "cons expects two arguments");
    } else if(args->next->obj != nil_obj &&
            args->next->obj->type != list_type) {
        printf("error: the second argument of cons must be a list\n");
        return error_init(type_err, "the second argument of cons must be a list");
    }

    struct Object* output;
    // TODO: check for empty list?
    if(args->next->obj == nil_obj) {
        output = list_init();
    } else {
        output = object_copy(args->next->obj);
    }

    list_prepend_object(output, args->obj);
    return output;
}

struct Object* lat_first(struct List* args) {
    if(args->obj == NULL || args->next != NULL ||
            args->obj->type != list_type) {
        printf("error in 'first'\n");
        return error_init_bare();
    }
    /*
    struct Object* obj = args->obj;
    struct List* list = obj->data.ptr;
    return list->obj;
    */
    return ((struct List*)args->obj->data.ptr)->obj;
}

struct Object* lat_rest(struct List* args) {
    if(args->obj == NULL || args->next != NULL) {
        printf("error in 'rest'\n");
        return error_init_bare();
    }
    struct Object* obj = args->obj;
    if(obj->type == list_type) {
        struct List* list = obj->data.ptr;
        list = list->next;
        struct Object* output = list_init();
        while(list != NULL) {
            struct Object* copy = object_copy(list->obj);
            list_append_object(output, copy);
            list = list->next;
        }
        return output;
    } else {
        return nil_obj;
    }
}

struct Object* lat_concat(struct List* args) {
    struct Object* output = list_init();
    while(args != NULL) {
        if(args->obj != NULL && args->obj->type == list_type) {
            struct List* list = args->obj->data.ptr;
            while(list != NULL) {
                struct Object* copy = object_copy(list->obj);
                list_append_object(output, copy);
                list = list->next;
            }
        } else {
            struct Object* copy = object_copy(args->obj);
            list_append_object(output, copy);
        }
        args = args->next;
    }
    return output;
}

struct Object* lat_list(struct List* args) {
    struct Object* output = list_init();
    while(args != NULL) {
        struct Object* copy = object_copy(args->obj);
        list_append_object(output, copy);
        args = args->next;
    }
    return output;
}

struct Object* lat_equals_value(struct List* args) {
    if(args == NULL || args->next == NULL) {
        return error_init(arg_err, "function requires at least two arguments");
    } else {
        struct Object* first = args->obj;
        args = args->next;
        while(args != NULL) {
            if(object_equals_value(first, args->obj)) {
                args = args->next;
            } else {
                return nil_obj;
            }
        }
        return true_obj;
    }
}

struct Object* lat_equals(struct List* args) {
    if(args == NULL || args->next == NULL) {
        return error_init(arg_err, "function requires at least two arguments");
    } else {
        struct Object* first = args->obj;
        args = args->next;
        while(args != NULL) {
            if(args->obj == first) {
                args = args->next;
            } else {
                return nil_obj;
            }
        }
        return true_obj;
    }
}

struct Object* lat_read(struct List* args) {
    if(args->obj == NULL || args->obj->type != string || args->next != NULL) {
        printf("error: read expects one string argument\n");
        return error_init_bare();
    }
    struct Object* output = read_string(args->obj->data.ptr);
    if(output == NULL) {
        printf("lat_read should not return NULL pointer\n");
        return nil_obj;
    } else {
        return output;
    }
}

struct Object* lat_eval(struct List* args) {
    if(args->obj == NULL || args->next != NULL) {
        printf("error: eval expects one argument\n");
        return error_init_bare();
    }
    struct Object* output = lat_evaluate(curr_env, args->obj);
    return output;
}

struct Object* lat_print(struct List* args) {
    while(args != NULL) {
        object_print_pretty(args->obj);
        args = args->next;
        if(args != NULL) {
            printf(" ");
        }
    }
    printf("\n");
    return nil_obj;
}

struct Object* lat_plus(struct List* args) {
    if(args == NULL) {
        printf("error: wrong number of arguments to +\n");
        return error_init(arg_err, "function requires at least one argument");
    }
    int value = 0;
    float fvalue = 0;
    int is_float = 0;
    while(args != NULL) {
        if(args->obj != NULL &&
                args->obj->type != int_type &&
                args->obj->type != float_type) {
            printf("error: wrong type of argument to +, expected int\n");
            return error_init(type_err, "function expects numeric arguments");
        } else if(args->obj->type == float_type && !is_float) {
            is_float = 1;
            fvalue = value;
        }

        if(is_float) {
            if(args->obj->type == float_type)
                fvalue += args->obj->data.float_type;
            else
                fvalue += args->obj->data.int_type;
        } else {
            value += args->obj->data.int_type;
        }
        args = args->next;
    }
    union Data data;
    if(is_float) {
        data.float_type = fvalue;
        return object_init(float_type, data);
    } else {
        data.int_type = value;
        return object_init(int_type, data);
    }
}

struct Object* lat_mult(struct List* args) {
    if(args == NULL) {
        printf("error: wrong number of arguments to *\n");
        return error_init_bare();
    }
    int value = 1;
    float fvalue = 0;
    int is_float = 0;
    while(args != NULL) {
        if(args->obj != NULL &&
                args->obj->type != int_type &&
                args->obj->type != float_type) {
            printf("error: wrong type of argument to *, expected int\n");
            return error_init_bare();
        } else if(args->obj->type == float_type && !is_float) {
            is_float = 1;
            fvalue = value;
        }

        if(is_float) {
            if(args->obj->type == float_type)
                fvalue *= args->obj->data.float_type;
            else
                fvalue *= args->obj->data.int_type;
        } else {
            value *= args->obj->data.int_type;
        }
        args = args->next;
    }
    union Data data;
    if(is_float) {
        data.float_type = fvalue;
        return object_init(float_type, data);
    } else {
        data.int_type = value;
        return object_init(int_type, data);
    }
}


void envir_insert_cfn(struct Object* (*fn_ptr)(struct List*), char* name) {
    union Data data = { .fn_ptr = fn_ptr };
    struct Object* fn = object_init(c_fn, data);
    envir_set(global_env, name, fn);
}

void env_init() {
    global_env = envir_init(128);

    union Data temp;
    temp.ptr = NULL;
    true_obj = object_init(symbol, temp);
    nil_obj = object_init(symbol, temp);
    envir_set(global_env, "t", true_obj);
    envir_set(global_env, "nil", nil_obj);

    envir_insert_cfn(&lat_cons, "cons");
    envir_insert_cfn(&lat_concat, "concat");
    envir_insert_cfn(&lat_list, "list");
    envir_insert_cfn(&lat_first, "first");
    envir_insert_cfn(&lat_rest, "rest");

    envir_insert_cfn(&lat_read, "read");
    envir_insert_cfn(&lat_eval, "eval");
    envir_insert_cfn(&lat_print, "print");

    envir_insert_cfn(&lat_plus, "+");
    envir_insert_cfn(&lat_mult, "*");
    envir_insert_cfn(&lat_equals, "eq");
    envir_insert_cfn(&lat_equals_value, "=");

    /*
    struct Object* tree = read_module("./core.lisp");
    struct List* exprs = tree->data.ptr;
    while(exprs != NULL) {
        lat_evaluate(global_env, exprs->obj);
        exprs = exprs->next;
    }
    */

    user_env = envir_init(128);
    user_env->outer = global_env;
    global_env->inner = user_env;
}
