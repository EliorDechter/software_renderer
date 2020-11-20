#ifndef PR_PARSER
#define PR_PARSER

#include "common.c"
#include <stdarg.h>

typedef struct String {
    u8 *data;
    u32 size;
    u32 null_terminated_size;
    u32 max_size;
} String;

typedef enum Num_type { num_type_int, num_type_float } Num_type;

typedef struct Number {
    union {
        float float_value;
        int int_value;
    };
    Num_type type;
} Number;

typedef struct Buffer_with_name {
    Buffer buffer;
    String name;
} Buffer_with_name;

typedef enum Token_type {
    token_type_unkown,
    
    token_type_open_brace,
    token_type_closed_brace,
    token_type_equal,
    token_type_comma,
    token_type_number,
    token_type_string,
    token_type_identifier,
    
    token_type_spacing,
    token_type_eof,
    token_type_comment,
    
    token_type_null_terminator
} Token_type;

typedef struct Token {
    String file_name;
    s32 column_number;
    s32 line_number;
    
    Token_type type;
    String string;
    Number num;
} Token;

typedef struct Tokenizer {
    char *file_name;
    u8 buffer_size;
    s32 column_num;
    s32 row_num;
    u8 *current_char;
    Token token;
    
    bool error;
} Tokenizer;

typedef struct String_iterator {
    String *string;
    char *current_char;
} String_iterator;


Tokenizer g_tokenizer;

void fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("FATAL: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    exit(1);
}

Buffer create_buffer(size_t size) {
    Buffer buffer = {0};
    buffer.data = malloc(size);
    return buffer;
}

Number create_number() {
    Number number = {0};
    return number;
}

void copy_string(String *dest, const String *source) {
    assert(dest->size <= source->size);
    memset(dest->data, 0, dest->max_size);
    for (int i = 0; i < source->null_terminated_size; ++i) {
        dest->data[i] = source->data[i];
    }
}


void fill_string(String *string, const char *c_string) {
    //TODO: assert this works?
    memset(string->data, 0, string->max_size);
    for (int i = 0; i < string->null_terminated_size; ++i) {
        string->data[i] = c_string[i];
    }
}


String create_string_with_data(size_t size, u8 *data) {
    String string = {
        .data = malloc(size + 1),
        .size = 0,
        .null_terminated_size = 1,
        .max_size = size + 1
    };
    
    fill_string(&string, data);
    
    return string;
}

String create_string() {
    String string = {
        .data = malloc(21),
        .size = 0,
        .null_terminated_size = 1,
        .max_size = 21
    };
    
    return string;
}


String create_string_and_fill(const char *c_string) {
    String string = create_string();
    fill_string(&string, c_string);
}


bool is_null_string(String *string) {
    if (string->data == NULL)
        return true;
    return false;
}

String create_null_string() {
    String string = {0};
    
    return string;
}

String create_string_with_data_pointer(int size, u8 *data) {
    String string = {
        .data = data,
        .size = 0,
        .null_terminated_size = 1,
        .max_size = size + 1
    };
    
    return string;
}

Buffer_with_name create_buffer_with_name(size_t buffer_size) {
    Buffer_with_name buffer_with_name = {
        .buffer = create_buffer(buffer_size),
        .name = create_string()
    };
    
    return buffer_with_name;
}


String load_file_to_string(u8 *file_name) {
    FILE *file = fopen(file_name, "r");
    if (!file) {
        printf("%s not found/n", file_name);
        return create_null_string();
    }
    
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *data = malloc(size + 1);
    fread(data, size, 1, file);
    
    data[size] = '\0';
    
    String string = {
        .data = data,
        size = size,
        .null_terminated_size = size + 1,
        .max_size = size + 1
    };
    
    fclose(file);
    
    return string;
}

bool is_char_new_line(char c) {
    if (c == '\n')
        return true;
    return false;
}

bool is_skippable_char(char c) {
    if (isspace(c))
        return true;
    return false;
}

void append_to_string(String *string, u8 value) {
    assert(string->size < 20);
    string->data[string->size++] = value;
}

Token create_token() {
    Token token = {
        .column_number = 0,
        .line_number = 0,
        .type = token_type_unkown,
        .string = create_string(),
        .num = create_number()
    };
    
    return token;
}

Tokenizer create_tokenizer(String string, char *file_name) {
    Tokenizer tokenizer = {
        .file_name = file_name,
        .row_num = 0,
        .column_num = 0,
        .current_char = string.data,
        .token = create_token()
    };
    
    return tokenizer;
}

void print_parser_error(Tokenizer *tokenizer, const char *format, ...) {
    //TODO: be wary of a possible buffer overflow
    char string_buffer[1000];
    printf(string_buffer, "Error (%d, %d): ", tokenizer->row_num, tokenizer->column_num);
    va_list args;
    va_start(args, format);
    vprintf(string_buffer, args);
    printf("\n");
    va_end(args);
}

u8 move_tokenizer_to_next_char(Tokenizer *tokenizer) {
    char *current_char = ++tokenizer->current_char;
    //assert(tokenizer->current_char);
    if (is_char_new_line(*current_char)) {
        tokenizer->row_num++;
        tokenizer->column_num = 0;
    }
    else {
        tokenizer->column_num++;
    }
    
    return *current_char;
}

Token fill_token(Tokenizer *tokenizer) {
    String string = create_string();
    char current_char = move_tokenizer_to_next_char(tokenizer);
    while(!is_skippable_char(current_char)) {
        append_to_string(&string, current_char);
        current_char = move_tokenizer_to_next_char(tokenizer);
    }
}

bool is_token_equal(Token t, char *str) {
    if (strcmp(t.string.data , str) == 0)
        return true;
    return false;
}

bool is_char_number(char c) {
    if (c >= 0 && c <= 9) {
        return true;
    }
    return false;
}

String_iterator create_string_iterator(String *string) {
    String_iterator iterator = {
        .string = string,
        .current_char = string->data
    };
    
    return iterator;
}

char *get_next_char(String_iterator *iterator) {
    char *current_char = iterator->current_char;
    current_char++;
    if ((uintptr_t)current_char < (uintptr_t)(iterator->string->data + iterator->string->size))
        return NULL;
    return current_char;
}

bool is_string_number(Tokenizer *tokenizer, String *string) {
    String_iterator iterator = create_string_iterator(string);
    char *current_char = get_next_char(&iterator);
    
    if (*current_char == '+' || *current_char == '-') {
        current_char = get_next_char(&iterator);
        if (!is_char_number(*current_char)) {
            print_parser_error(tokenizer, "+ or - must be followed by a number");
            return false;
        }
    }
    else if (!is_char_number(*current_char)) {
        return false;
    }
    
    bool found_point = false;
    while(current_char = get_next_char(&iterator)) {
        if (!is_char_number(*current_char)) {
            print_parser_error(tokenizer, "the string is neither a number nor and identifier");
            return false;
        }
        else if (*current_char == '.') {
            if (found_point) {
                print_parser_error(tokenizer, "only one dot is allowed per number");
                return false;
            }
            found_point = true;
        }
    }
    
    return true;
}

void copy_part_of_string(String *source_string, String *dest_string, int range_start, int range_end) {
    String_iterator iterator = create_string_iterator(source_string);
    char *current_char;
    int index = range_start;
    while (current_char == get_next_char(&iterator) && index < range_end) {
        dest_string->data[index++] = *current_char;
    }
}

void split_string_by_index(String *string, char seperator, String *out_string0, String *out_string1) {
    
}

void split_string_by_char(String *string, char seperator, String *out_string0, String *out_string1) {
    int counter = 0;
    String_iterator iterator = create_string_iterator(string);
    char *current_char;
    while (current_char = get_next_char(&iterator)) {
        counter++;
        if (*current_char == seperator) {
            break;
        }
    }
    
    copy_part_of_string(string, out_string0, 0, counter);
    copy_part_of_string(string, out_string1, counter + 1, string->size);
}

bool is_char_contained_in_string(String *string, char c) {
    String_iterator iterator = create_string_iterator(string);
    char *current_char;
    while(current_char = get_next_char(&iterator)) {
        if (*current_char == c)
            return true;
    }
    return false;
}

Number convert_string_to_number(String *string) {
    String_iterator iterator = create_string_iterator(string);
    char *current_char = get_next_char(&iterator);
    int sign = 1;
    if (*current_char == '+') 
        sign = 1;
    else if (*current_char == '-')
        sign = -1;
    
    
    Number num = {0};
    
    if (is_char_contained_in_string(string, '.')) {
        String first_half = create_string();
        String second_half = create_string();
        split_string_by_char(string, '.', &first_half, &second_half);
        String_iterator first_half_iterator = create_string_iterator(&first_half);
        String_iterator second_half_iterator = create_string_iterator(&second_half);
        char *current_char;
        int multiplier = pow(10,first_half.size);
        int first_half_value = 0;
        int second_half_value = 0;
        while (current_char = get_next_char(&first_half_iterator)) {
            first_half_value += multiplier * (*current_char);
            multiplier *= 0.1;
        }
        multiplier = 0.1;
        while (current_char = get_next_char(&second_half_iterator)) {
            second_half_value += multiplier * (*current_char);
            multiplier *= 0.1;
        }
        
        num.int_value = (first_half_value  + second_half_value) * sign;
        num.type = num_type_int;
    }
    else {
        int multiplier = pow(10, string->size);
        int value = 0;
        while (current_char = get_next_char(&iterator)) {
            value += multiplier * (*current_char);
            multiplier *= 0.1;
        }
        
        num.int_value = value * sign;
        num.type = num_type_int;
    }
    
    return num;
}

bool is_string_identifier(String *string) { return false;/*TODO*/ }

Token get_next_token(Tokenizer *tokenizer) {
    Token token = fill_token(tokenizer);
    
    u8 current_char = *tokenizer->current_char;
    
    switch(current_char) {
        case '\0': { token.type = token_type_null_terminator; return token;}
        
        case '{': { token.type = token_type_open_brace; return token;}
        case '}': { token.type = token_type_closed_brace; return token;}
        case '=': { token.type = token_type_equal; return token;}
        case ',': { token.type = token_type_comma; return token;}
    }
    
    if (is_string_number(tokenizer, &token.string)) {
        token.type = token_type_number;
        int integer_value;
        float float_value;
        token.num = convert_string_to_number(&token.string);
        
        return token;
    }
    
    if (is_string_identifier(&token.string)) {
        token.type = token_type_identifier;
        return token;
    }
}


char *convert_token_type_to_string(Token_type type) {
    switch(type) {
        case token_type_open_brace: { return "{";}
        case token_type_closed_brace: { return "}";}
        case token_type_identifier: { return "identifier";}
        case token_type_number: { return "number";}
        case token_type_comma: { return "comma";}
    }
}

bool require_next_token(Tokenizer *tokenizer, Token_type required_type) {
    Token token = get_next_token(tokenizer);
    if (token.type != required_type) {
        print_parser_error(tokenizer, "expected %s", convert_token_type_to_string(required_type));
        return false;
    }
    
    return true;
}

bool require_and_get_next_token(Tokenizer *tokenizer, Token_type required_type, Token *out_token) {
    Token token = get_next_token(tokenizer);
    if (token.type != required_type) {
        print_parser_error(tokenizer, "expected %s", convert_token_type_to_string(required_type));
        out_token = NULL;
        return false;
    }
    
    *out_token = token;
    return true;
}

bool parse_test_vertices_file(char *file_name, Buffer_with_name *vertex_buffer) {
    String text = load_file_to_string(file_name);
    assert(!is_null_string(&text));
    
    Tokenizer tokenizer = create_tokenizer(text, file_name);
    Token token = create_token();
    *vertex_buffer = create_buffer_with_name(100);
    
    bool stop_parsing = false;
    while (!stop_parsing) {
        if (!require_and_get_next_token(&tokenizer, token_type_identifier, &token)) {
            return false;
        }
        
        copy_string(&vertex_buffer->name, &token.string);
        
        if(!require_next_token(&tokenizer, token_type_equal)) {
            return false;
        }
        
        if (!require_next_token(&tokenizer, token_type_open_brace)) {
            return false;
        }
        
        token = get_next_token(&tokenizer);
        if (token.type == token_type_closed_brace) {
            continue;
        }
        
        bool stop_parsing_vertex_buffer = false;
        while (!stop_parsing_vertex_buffer) {
            token = get_next_token(&tokenizer);
            if (token.type == token_type_number) {
                //add_to_buffer(&vertex_buffer->data, token.num.float_value);
                
                token = get_next_token(&tokenizer);
                if (token.type == token_type_comma) {
                    //skip
                }
                else if (token.type == token_type_closed_brace) {
                    stop_parsing_vertex_buffer = true;
                }
                else {
                    print_parser_error(&tokenizer, "expected %s or %s", convert_token_type_to_string(token_type_comma), convert_token_type_to_string(token_type_closed_brace));
                    return false;
                }
            }
            else {
                //print_parser_error(tokenizer, "expected %s", convert_token_type_to_string(token_type_identifier));
                return false;
            }
        }
    }
}

bool is_literal_string_equal(const char *a, const char *b) {
    if (strcmp(a, b) == 0)
        return true;
    return false;
}

void test_parser() {
    String text = load_file_to_string("test_vertices.pav");
    assert(!is_null_string(&text));
    init_tokenizer();
    while(
}

#endif 