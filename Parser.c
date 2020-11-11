#ifndef PR_PARSER
#define PR_PARSER

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

Buffer_with_name create_buffer_with_name(size_t buffer_size) {
    Buffer_with_name buffer_with_name = {
        .buffer = create_buffer(buffer_size),
        .name = create_string()
    };
    
    return buffer_with_name;
}


Number create_number() {
    Number number = {0};
    return number;
}

String create_string_by_size(size_t size) {
    String string = {
        .data = malloc(size + 1),
        .size = 0,
        .null_terminated_size = 1,
        .max_size = size + 1
    };
    
    return string;
}

String create_string() {
    return create_string_by_size(20);
}

void copy_string(String *str0, const String *str1) {
    assert(str0.size <= str1.size);
    memset(string.data, 0, string.max_size);
    for (int i = 0; i < str1.null_terminated_size; ++i) {
        str0[i] = str1[i];
    }
}

void fill_string(String *string, const char *c_string) {
    //TODO: assert this works?
    memset(string.data, 0, string.max_size);
    for (int i = 0; i < string.null_terminated_size; ++i) {
        string[i] = c_string[i];
    }
}

String create_string_and_fill(const char *c_string) {
    String string = create_string();
    fill_string(&string, c_string);
}

String load_file_to_string(u8 *file_name) {
    FILE *file = fopen(file_name, "r");
    if (!file) {
        pritnf("%s not found/n", file_name);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    String string = create_string_by_size((size_t)size);
    
    char *data = malloc(size + 1);
    *data = 
        fread(data, size, 1, file);
    
    fclose(file);
    Buffer buffer = {.size = size, .data = data };
    
    return buffer;
}

typedef struct Tokenizer {
    char *file_name;
    u8 buffer_size;
    s32 column_number;
    s32 line_number;
    //stream
    
    //char *input;
    //char at[2];
    u8 *current_char;
    
    bool error;
} Tokenizer;

typedef enum Token_type {
    token_type_unkown,
    
    token_type_open_brace,
    token_type_closed_brace,
    token_type_number,
    token_type_string,
    token_type_identifier,
    
    token_type_spacing,
    token_type_eof,
    token_type_comment,
    
    token_type_null_terminator
} Token_type;

typedef struct Token {
    string file_name;
    s32 column_number;
    s32 line_number;
    
    Token_type type;
    String string;
    Number num;
} Token;

bool is_char_new_line(char c) {
    if (c == '\n')
        return true;
    return false;
}

bool is_skippable_char(char c) {
    if (isspace(c))
        return truel;
    return false;
}

void append_to_string(String *string, u8 value) {
    assert(string->size < 20);
    string->data[string->size++] = value;
}

Token create_token(const Tokenizer *tokenizer) {
    Token token = {
        .file_name = tokenizer->file_name,
        .column_number = 0,
        .line_number = 0,
        .type = token_type_unkown,
        .string = create_string(),
        .number = create_number();
    };
    
    return token;
}

Tokenizer create_tokenizer(String string, const char *file_name) {
    Tokenizer tokenizer = {
        .file_name = file_name,
        .column_number = 0,
        .line_number = 0,
        .current_char = string.data,
    };
    
    return tokenizer;
}

void print_parser_error(Tokenizer tokenizer, char *message) {
    printf("Error (%d, %d): %s", tokenizer.row_num, tokenizer.column_num, message);
}

u8 move_tokenizer_to_next_char(Tokenizer *tokenizer) {
    char *current_char = ++tokenizer->current_char;
    //assert(tokenizer->current_char);
    if (is_char_new_line(current_char)) {
        tokenizer->current_line++;
        tokenizer->current_column = 0;
    }
    else {
        tokenizer->current_column++;
    }
    
    return *current_char;
}

Token fill_token(tokenizer) {
    String string = create_string();
    char current_char = move_tokenizer_to_next_char(tokenizer);
    while(!is_skippable_char(current_char)) {
        append_to_string(current_char);
        current_char = move_tokenizer_to_next_char(tokenizer);
    }
}

bool is_token_equal(Token t, char *str) {
    if (strcmp(t.string.data , str) == 0)
        return true;
    return false;
}

bool is_string_number() {
    
}

bool is_char_number(char c) {
    if (c >= 0 && c <= 9) {
        return true
    }
    return false;
}

typedef struct String_iterator {
    String *string;
    char *current_char;
} Strin_iterator;

void create_string_iterator(String *string) {
    String_iterator iterator = {
        .string = string,
        .current_char = string->data[0]
    };
    
    return iterator;
}

char *get_next_char(String_iterator *iterator) {
    char *current_char = iterator->current_char;
    current_char++;
    if (current_char == '\0')
        return NULL;
    return current_char;
}

bool is_string_number(String *string) {
    String_iterator iterator = create_string_iterator(string);
    char *current_char = get_next_char(iterator);
    
    if (*char == '+' || *char == '-') {
        current_char = get_next_char(iterator);
        if (!is_char_number(*current_char)) {
            print_parser_error("+ or - must be followed by a number");
            return false;
        }
    }
    else if (!is_char_number(*current_char)) {
        return false;
    }
    
    bool found_point = false;
    while(current_char = get_next_char(iterator)) {
        if (!is_char_number(*current_char)) {
            print_parser_error("the string is neither a number nor and identifier");
            return false;
        }
        else if (*current_char == '.') {
            if (found_point) {
                print_parser_error("only one dot is allowed per number");
                return false;
            }
            found_point = true;
        }
    }
    
    return true;
}

void copy_part_of_string(String *source_string, String *dest_string, int range_start, int range_end) {
    String_iterator iterator = create_new_iterator(source_string);
    char *current_char;
    int index = range_start;
    while (current_char == get_next_char(iterator) && index < range_end) {
        dest_string[index++] = *current_char;
    }
}

void split_string_by_index(String *string, char seperator, String *out_string0, String *out_string1) {
    
}

void split_string_by_char(String *string, char seperator, String *out_string0, String *out_string1) {
    int counter = 0;
    String_iterator iterator = create_new_iterator(string);
    char *current_char;
    while (current_char = get_next_char(iterator)) {
        counter++;
        if (*current_char == seperator) {
            break;
        }
    }
    
    copy_part_of_string(string, out_string0, 0, counter);
    copy_part_of_string(string, out_string1, counter + 1, string.size);
}

bool is_char_contained_in_string(String *string, char c) {
    String_iterator iterator = create_string_iterator(string);
    char *current_char;
    while(current_char = get_next_char(iterator)) {
        if (*current_char == c)
            return true;
    }
    return false;
}

Number convert_string_to_number(String *string) {
    String_iterator iterator = create_string_iterator(string);
    char *current_char = get_next_char(iterator);
    if (*current_char == '+') 
        sign = 1;
    else if (*current_char == '-')
        sign = -1;
    
    integer_value = float_value = NULL;
    
    Number num = {0};
    
    if (is_char_contained_in_string('.')) {
        String first_half = create_string();
        String second_half = create_string();
        split_string_by_char(string, '.', &first_half, &second_half);
        String_iterator first_half_iterator = create_string_iterator(first_half);
        String_iterator second_half_iterator = create_string_iterator(second_half);
        char *current_char;
        int multiplier = pow(10,first_half.size);
        int first_half_value = second_half_value = 0;
        while (current_char = get_next_char(first_half_iterator)) {
            first_half_value += multiplier * (*current_char);
            multiplier *= 0.1;
        }
        multiplier = 0.1;
        while (current_char = get_next_char(second_half_iterator)) {
            second_half_value += multiplier * (*current_char);
            multiplier *= 0.1;
        }
        
        num.integer_value = (first_half_value  + second_half_value) * sign;
        num.type = num_type_integer;
    }
    else {
        int multiplier = pow(10,first_half.size);
        int value = 0;
        while (current_char = get_next_char(iterator)) {
            value += multiplier * (*current_char);
            multiplier *= 0.1;
        }
        
        num.integer_value = value * sign;
        num.type = num_type_integer;
    }
    
    return num;
}

Token get_next_token(Tokenizer *tokenizer) {
    Token token = fill_token(tokenizer);
    
    u8 current_char = *tokeinzer->current_char;
    
    switch(current_char) {
        case '\0': { token.type = token_type_end_of_buffer; return;}
        
        case '{': { token.type = token_type_open_brace; return token;}
        case '}': { token.type = token_type_closed_brace; return token;}
        case '=': { token.type = token_type_equal; return token;}
        case ',': { token.type = token_type_comma; return token;}
        case 'identifier': { token.type = token_type_identifier; return token;}
    }
    
    if (is_string_number(token.string)) {
        token.type = token_type_integer;
        int integer_value;
        float float_value;
        token.num = convert_string_to_number(token.string);
        
        return token;
    }
    
    if (is_string_identifier(token.string)) {
        token.type = token_type_identifier;
        return token;
    }
}

bool require_next_token(Tokenizer *tokenizer, Token_type required_type) {
    Token token = get_next_token(&tokenizer);
    if (token.type != required_type) {
        print_parser_error(tokenizer, "expected %s", convert_token_type_to_string(expected_type));
        return false;
    }
    
    return true;
}

bool require_and_get_next_token(Tokenizer *tokenizer, Token_type required_type, Token *out_token) {
    Token token = get_next_token(&tokenizer);
    if (token.type != required_type) {
        print_parser_error(tokenizer, "expected %s", convert_token_type_to_string(expected_type));
        out_token = NULL;
        return false;
    }
    
    *out_token = token;
    return true;
}

Buffer create_buffer(size_t buffer_size) {
    Buffer buffer = {0};
    buffer.data = malloc(size);
    return buffer;
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

bool parse_test_vertices_file(const char *file_name, Buffer_with_name *vertex_buffer) {
    String text = load_file_to_string(file_name);
    
    Tokenizer tokenizer = create_tokenizer(text, file_name);
    Token token = create_token(&tokenizer);
    *vertex_buffer = create_buffer_with_name(100);
    
    bool stop_parsing = false;
    while (!stop_parsing) {
        if (!require_and_get_next_token(&tokenizer, token_type_identifier, &token)) {
            return false;
        }
        
        copy_string(&vertex_buffer->name, token.string);
        
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
                add_to_buffer(&vertex_buffer->data, token.num.float_value);
                
                token = get_next_token(&tokenizer);
                if (token.type == token_type_comma) {
                    //skip
                }
                else if (token.type == token_type_closed_brace) {
                    stop_parsing_vertex_buffer = true;
                }
                else {
                    print_parser_error(tokenizer, "expected %s or %s", convert_token_type_to_string(token_type_comma), convert_token_type_to_string(token_type_closed_brace));
                    return false;
                }
            }
            else {
                print_parser_error(tokenizer, "expected %s", convert_token_type_to_string(token_type_identifier));
                return false;
            }
        }
    }
}

#endif PR_PARSER