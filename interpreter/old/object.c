#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "list.h"
#include "lang.h"
#include "garbage.h"
#include "error.h"

#include "object.h"

struct Object* object_init_type(enum object_type type) {
    struct Object* obj = malloc(sizeof(struct Object));
    obj->type = type;
    obj->data.ptr = NULL;
    obj->marked = 0;
    gc_insert_object(obj);
    return obj;
}

struct Object* object_init(enum object_type type, union Data data) {
    struct Object* obj = malloc(sizeof(struct Object));
    obj->type = type;
    obj->data = data;
    obj->marked = 0;
    gc_insert_object(obj);
    return obj;
}

struct Object* object_symbol_init(char* str) {
    char* str_copy = malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(str_copy, str);
    union Data data = { .ptr = str_copy };
    return object_init(symbol, data);
}

struct Object* object_copy(struct Object* obj) {
    if(obj == NULL) {
        return NULL;
    } else if(obj == true_obj || obj == nil_obj) {
        return obj;
    }

    struct Object* source = obj;
    struct Object* output = object_init_type(source->type);
    if(source->type == char_type ||
            source->type == float_type ||
            source->type == int_type ||
            source->type == c_fn) {
        // direct copy
        output->data = obj->data;
    } else if(source->type == symbol || source->type == string) {
        // string copy
        int length = strlen(source->data.ptr);
        char* str = malloc(sizeof(char) * (length + 1));
        strcpy(str, obj->data.ptr);
        output->data.ptr = str;
    } else if(source->type == list_type) {
        struct Object* clone = list_init();
        struct List* list = source->data.ptr;
        while(list != NULL) {
            struct Object* lclone = object_copy(list->obj);
            list_append_object(clone, lclone);
            list = list->next;
        }
        return clone;
    } else if(obj->type == func_type || obj->type == macro_type) {
        printf("implement object_copy for functions\n");
        return NULL;
    }
    return output;
}

void object_copy2(struct Object** dest, struct Object* src) {
    if(src == NULL) {
        *dest = NULL;
    } else if(src == true_obj || src == nil_obj) {
        *dest = src;
    }

    *dest = object_init_type(src->type);
    if(src->type == char_type || src->type == int_type || src->type == c_fn) {
        (*dest)->data = src->data;
    } else if(src->type == symbol || src->type == string) {
        int length = strlen(src->data.ptr);
        char* str = malloc(sizeof(char) * (length + 1));
        strcpy(str, src->data.ptr);
        (*dest)->data.ptr = str;
    } else if(src->type == list_type) {
        (*dest)->data.ptr = list_bare_init();
        struct List* dest_lst = (*dest)->data.ptr;
        struct List* src_lst = src->data.ptr;
        while(src_lst != NULL) {
            dest_lst = list_bare_append(dest_lst, src_lst->obj);
            src_lst = src_lst->next;
        }
    } else if(src->type == func_type || src->type == macro_type) {
        printf("implement object_copy for functions\n");
    }
}

int object_equals_char(struct Object* obj, char c) {
    if(obj != NULL && obj->type == char_type && obj->data.char_type == c) {
        return 1;
    } else {
        return 0;
    }
}

int object_equals_symbol(struct Object* obj, char* str) {
    if(obj != NULL && obj->type == symbol &&
            strcmp(obj->data.ptr, str) == 0) {
        return 1;
    } else {
        return 0;
    }
}

int object_equals_string(struct Object* obj, char* str) {
    if(obj != NULL && obj->type == string &&
            strcmp(obj->data.ptr, str) == 0) {
        return 1;
    } else {
        return 0;
    }
}

int object_is_nonempty_list(struct Object* obj) {
    if(obj != NULL && obj->type == list_type) {
        struct List* list = obj->data.ptr;
        if(list->obj != NULL) {
            return 1;
        }
    }
    return 0;
}

int object_equals_value(struct Object* a, struct Object* b) {
    if(a == b) {
        // also covers if both a and b are NULL
        return 1;
    } else if(a == NULL || b == NULL) {
        return 0;
    } else if(a->type != b->type) {
        return 0;
    }

    if(int_type == a->type) {
        return a->data.int_type == b->data.int_type ? 1 : 0;
    } else if(float_type == a->type) {
        return a->data.float_type == b->data.float_type ? 1 : 0;
    } else if(char_type == a->type) {
        return a->data.char_type == b->data.char_type ? 1 : 0;
    } else if(symbol == a->type || string == a->type) {
        return strcmp(a->data.ptr, b->data.ptr) == 0 ? 1 : 0;
    } else {
        printf("warning: equality not defined for lists\n");
        return a->data.ptr == b->data.ptr ? 1 : 0;
    }
    // TODO: list comparison
}

void object_mark(struct Object* obj) {
    if(obj == NULL || obj->marked)
        return;

    obj->marked = 1;
    if(obj->type == list_type) {
        struct List* list = (struct List*) obj->data.ptr;
        while(list != NULL) {
            object_mark(list->obj);
            list = list->next;
        }
    } else if(obj->type == func_type || obj->type == macro_type) {
        struct Func* func = (struct Func*)obj->data.ptr;
        object_mark(func->args);
        object_mark(func->expr);
    }
}

void object_free(struct Object* obj) {
    if(obj == NULL)
        return;

    switch(obj->type) {
        case symbol:
        case string:
            free(obj->data.ptr);
            break;

        case c_fn:
            printf("warning: attemtped to free function defined in c\n");
            break;
        default:
            break;
    }

    if(list_type == obj->type) {
        // printf("freeing list\n");
        struct List* list = (struct List*) obj->data.ptr;
        if(list != NULL) {
            struct List* next = list->next;
            while(list != NULL) {
                free(list);
                list = next;
                if(next != NULL)
                    next = next->next;
            }
        }
    } else if(func_type == obj->type || macro_type == obj->type) {
        printf("freeing function / macro\n");
        free(obj->data.ptr);
    }
    free(obj);
}

void object_print_type(enum object_type type) {
    switch(type) {
        case symbol:
            printf("symbol");
            break;
        case string:
            printf("string");
            break;
        case char_type:
            printf("char");
            break;
        case int_type:
            printf("int");
            break;
        case float_type:
            printf("float");
            break;
        case list_type:
            printf("list");
            break;
        case c_fn:
            printf("c_fn");
            break;
        case func_type:
            printf("func");
            break;
        case macro_type:
            printf("macro");
            break;
        case error_type:
            printf("error");
            break;
        default:
            printf("unknown type");
    }
}

static void print_literal(struct Object* obj) {
    char* str = (char*) obj->data.ptr;
    if(obj->type == string)
        printf("\"");
    while(*str != '\0') {
        if(*str == '\n') {
            printf("\\n");
        } else if( *str == '\t') {
            printf("\\t");
        } else if(*str == '\r') {
            printf("\\r");
        } else {
            printf("%c", *str);
        }
        str ++;
    }
    if(obj->type == string)
        printf("\"");
}

void object_print_string(struct Object* obj) {
    if(obj == NULL) {
        printf("[null]");
    } else if(list_type == obj->type) {
        printf("(");
        struct List* node = obj->data.ptr;
        if(node != NULL && node->obj != NULL) {
            while(node != NULL) {
                object_print_string(node->obj);
                node = node->next;
                if(node != NULL) {
                    printf(" ");
                }
            }
        }
        printf(")");
    } else if(true_obj == obj) {
        printf("t");
    } else if(nil_obj == obj) {
        printf("nil");
    } else if(string == obj->type || symbol == obj->type) {
        // string based types
        // printf("\"%s\"", (char*) obj->data.ptr);
        print_literal(obj);
    } else if(char_type == obj->type) {
        printf("%c", obj->data.char_type);
    } else if(int_type == obj->type) {
        printf("%d", obj->data.int_type);
    } else if(float_type == obj->type) {
        printf("%f", obj->data.float_type);
    } else if(c_fn == obj->type) {
        printf("c_fn<%p>", obj->data.ptr);
    } else if(func_type == obj->type) {
        printf("fn<%p>", obj->data.ptr);
    } else if(macro_type == obj->type) {
        printf("macro<%p>", obj->data.ptr);
    } else if(error_type == obj->type) {
        struct Error* err = obj->data.ptr;
        printf("%d error: %s", err->type, err->message);
    } else {
        printf("%p", obj->data.ptr);
    }
}

void object_print_pretty(struct Object* obj) {
    if(obj->type != string) {
        object_print_string(obj);
    } else {
        printf("%s", (char*) obj->data.ptr);
    }
}

void object_print_debug(struct Object* obj, int indent) {
    for(int i = 0; i < indent; i ++) {
        printf("  ");
    }
    if(obj == NULL) {
        printf("[ NULL OBJECT ]\n");
        return;
    }
    printf("adr %p:\n", (void*) obj);

    for(int i = 0; i < indent; i ++) {
        printf("  ");
    }
    object_print_type(obj->type);
    printf("\n");

    for(int i = 0; i < indent; i ++) {
        printf("  ");
    }
    if(obj->marked) {
        printf("mark\n");
    } else {
        printf("no mark\n");
    }

    for(int i = 0; i < indent; i ++) {
        printf("  ");
    }
    switch(obj->type) {
        case symbol:
            printf("data: %s\n", (char*) obj->data.ptr);
            break;
        case string:
            printf("data: %s\n", (char*) obj->data.ptr);
            break;
        case char_type:
            printf("data: %c\n", obj->data.char_type);
            break;
        case int_type:
            printf("data: %d\n", obj->data.int_type);
            break;
        case list_type:
            printf("data:\n");
            object_print_debug(obj->data.ptr, indent + 1);
            indent ++;
            struct List* list = (struct List*) obj->data.ptr;
            while(list != NULL) {
                for(int i = 0; i < indent; i ++) {
                    printf("  ");
                }
                printf("node addr: %p\n", (void *) list);
                object_print_debug(list->obj, indent + 1);
                list = list->next;
            }
            break;
        default:
            printf("data: %p\n", obj->data.ptr);
    }
}

void object_debug(struct Object* obj) {
    object_print_debug(obj, 0);
    printf("\n");
}
