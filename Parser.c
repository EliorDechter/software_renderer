
Buffer load_file_to_buffer(u8 *file_name) {
    FILE *file = fopen(file_name, "r");
    if (!file) {
        pritnf("%s not found/n", file_name);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *data = malloc(size); //TODO: how to use a variable sized allocation inside the perm allocator
    fread(data, size, 1, file);
    
    fclose(file);
    Buffer buffer = {.size = size, .data = data };
    
    return buffer;
}

typedef struct Tokenizer {
    char *file_name;
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
    
    //token_type_end_of_stream
} Token_type;

typedef struct Token {
    string file_name;
    s32 column_number;
    s32 line_number;
    
    Token_type type;
    String string;
    f32 float_value;
    s32 int_value;
} Token;

Token get_raw_token(Tokenizer *tokenizer) {
    Token token = {0};
    token.file_name = tokenizer->file_name;
    token.column_number = tokenizer->column_number;
    token.line_number = tokenizer->line_number;
    token.text = tokenizer->input;
    
    char c = tokenizer->at[0];
    adva
}

u8 move_tokenizer_to_next_char(Tokenizer *tokenizer) {
    //TODO: fix once we know the proper line and column
    tokenizer->column_number ++;
    tokenizer->line_number++;
    
    
}

Token get_next_token(Tokenizer *tokenizer) {
    Token token = {0};
    
    u8 current_char = *tokeinzer->current_char;
    switch(current_char) {
        case '\0': { token.type = token_type_end_of_buffer; break;}
        
        case '{': { token.type = token_type_open_brace; break;}
        case '}': { token.type = token_type_closed_brace; break;}
        case '=': { token.type = token_type_equal; break;}
        case ',': { token.type = token_type_comma; break;}
        case 'identifier': { token.type = token_type_identifier; break;}
    }
    if (current_char == '+' || current_char == '-') {
        u8 sign = current_char;
        ++tokenizer->current_char;
        current_char = *tokenizer->current_char;
        if (!(current_char >= 0 && current_char <= 9)) {
            printf("missing a number after sign at (row: %d column: %d)", tokenizer->row_number, tokenizer->column_number);
        }
        
        String string_number; //TODO: Fix
        for (int i = 0; current_char >= 0 && current_char <= 9; ++i) {
            append_to_string(&string_numer, move_tokenizer_to_next_char(tokenizer));
        }
        
        s32 num = convert_string_to_number(string);
    }
    else if (current_char >= 0 && current_char <= 9) {
        u8 sign = current_char;
        ++tokenizer->current_char;
        current_char = *tokenizer->current_char;
        if (!(current_char >= 0 && current_char <= 9)) {
            printf("missing a number after sign at (row: %d column: %d)", tokenizer->row_number, tokenizer->column_number);
        }
        
        String string_number; //TODO: Fix
        for (int i = 0; current_char >= 0 && current_char <= 9; ++i) {
            append_to_string(&string_numer, move_tokenizer_to_next_char(tokenizer));
        }
        
        s32 num = convert_string_to_number(string);
    }
    else if ((current_char >= 'a' && current_char <= 'z') || (current_char >= 'A' && current_char <= 'Z')) {
        //parse string
    }
}

String create_string() {
    String string = {
        .data = malloc(20),
        .size = 0
    };
    
    return string;
}

Token create_token() {
    Token token = {
        .file_name = tokenizer->file_name,
        .column_number = 0,
        .line_number = 0,
        
        .type = token_type_unkown,
        .string = create_string(),
        float_value = 0,
        int_value = 0
    };
    
    return token;
}

Tokenizer create_tokenizer(Buffer buffer, String string) {
    Tokenizer tokenizer = {
        .file_name = file_name,
        .column_number = 1,
        .line_number = 1,
        .current_char = buffer.data,
    };
    
    return tokenizer;
}

void print_parser_error(Tokenizer tokenizer, char *message) {
    printf("Error (%d, %d): %s", tokenizer.row_num, tokenizer.column_num, message);
}

char *convert_token_type_to_string(Token_type token_type) {
    //todo
    return NULL;
}

Token get_next_token_with_expectation(Tokenizer *tokenizer, Token_type expected_type) {
    Token token = get_next_token(&tokenizer);
    if (token.type != toke_type_open_brace) {
        print_parser_error(tokenizer, "expected %s", convert_token_type_to_string(expected_type));
        exit(1);
    }
}

void parse_test_vertices_file(const char *file_name) {
    Buffer buffer = load_file_to_buffer(file_name);
    
    Token token = create_token();
    Buffer vertex_buffer = {0};
    char *buffer_name = malloc(20);; //TODO: allocate
    //bool get_next_vertex_buffer = true;
    while (1) {
        
        token = get_next_token_with_expectation(&tokenizer, token_type_identifier);
        char *buffer_name = token.string;
        
        get_next_token_with_expectation(&tokenizer, token_type_equal);
        get_next_token_with_expectation(&tokenizer, token_type_open_brace);
        
        token = get_next_token(&tokenizer);
        if (token.type == token_type_closed_brace) {
            
        }
        
        while (1) {
            token = get_next_token(&tokenizer);
            if (token.type == token_type_number) {
                add_to_buffer(&vertex_buffer, token.float_num);
                
                token = get_next_token(&tokenizer);
                if (token.type == token_type_comma) {
                    
                }
                if (token.type == token_type_brace) {
                    break;
                }
                else {
                    //error
                }
            }
            else {
                //error
            }
        }
        
    }
}