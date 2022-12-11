%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */
 /* TODO: Put your lab2 code here */

%x COMMENT STR IGNORE ESCAPE

%%

 /*
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  *
  * All the tokens:
  *   Parser::ID
  *   Parser::STRING
  *   Parser::INT
  *   Parser::COMMA
  *   Parser::COLON
  *   Parser::SEMICOLON
  *   Parser::LPAREN
  *   Parser::RPAREN
  *   Parser::LBRACK
  *   Parser::RBRACK
  *   Parser::LBRACE
  *   Parser::RBRACE
  *   Parser::DOT
  *   Parser::PLUS
  *   Parser::MINUS
  *   Parser::TIMES
  *   Parser::DIVIDE
  *   Parser::EQ
  *   Parser::NEQ
  *   Parser::LT
  *   Parser::LE
  *   Parser::GT
  *   Parser::GE
  *   Parser::AND
  *   Parser::OR
  *   Parser::ASSIGN
  *   Parser::ARRAY
  *   Parser::IF
  *   Parser::THEN
  *   Parser::ELSE
  *   Parser::WHILE
  *   Parser::FOR
  *   Parser::TO
  *   Parser::DO
  *   Parser::LET
  *   Parser::IN
  *   Parser::END
  *   Parser::OF
  *   Parser::BREAK
  *   Parser::NIL
  *   Parser::FUNCTION
  *   Parser::VAR
  *   Parser::TYPE
  */

 /* reserved words */
"while" {adjust(); return Parser::WHILE;}
"for" {adjust(); return Parser::FOR;}
"to" {adjust(); return Parser::TO;}
"break" {adjust(); return Parser::BREAK;}
"let" {adjust(); return Parser::LET;}
"in" {adjust(); return Parser::IN;}
"end" {adjust(); return Parser::END;}
"function" {adjust(); return Parser::FUNCTION;}
"var" {adjust(); return Parser::VAR;}
"type" {adjust(); return Parser::TYPE;}
"array" {adjust(); return Parser::ARRAY;}
"if" {adjust(); return Parser::IF;}
"then" {adjust(); return Parser::THEN;}
"else" {adjust(); return Parser::ELSE;}
"do" {adjust(); return Parser::DO;}
"of" {adjust(); return Parser::OF;}
"nil" {adjust(); return Parser::NIL;}
"," {adjust(); return Parser::COMMA;}
":" {adjust(); return Parser::COLON;}
";" {adjust(); return Parser::SEMICOLON;}
"(" {adjust(); return Parser::LPAREN;}
")" {adjust(); return Parser::RPAREN;}
"[" {adjust(); return Parser::LBRACK;}
"]" {adjust(); return Parser::RBRACK;}
"{" {adjust(); return Parser::LBRACE;}
"}" {adjust(); return Parser::RBRACE;}
"." {adjust(); return Parser::DOT;}
"+" {adjust(); return Parser::PLUS;}
"-" {adjust(); return Parser::MINUS;}
"*" {adjust(); return Parser::TIMES;}
"/" {adjust(); return Parser::DIVIDE;}
"=" {adjust(); return Parser::EQ;}
"<>" {adjust(); return Parser::NEQ;}
"<" {adjust(); return Parser::LT;}
"<=" {adjust(); return Parser::LE;}
">" {adjust(); return Parser::GT;}
">=" {adjust(); return Parser::GE;}
"&" {adjust(); return Parser::AND;}
"|" {adjust(); return Parser::OR;}
":=" {adjust(); return Parser::ASSIGN;}

[[:digit:]]+ {adjust(); return Parser::INT;}
[[:alpha:]][[:alnum:]_]* {adjust(); return Parser::ID;}


\" {
  adjust();
  begin(StartCondition__::STR);
}
<STR> {
  ([[:print:]]{-}[\"\\]{+}[[:space:]])+ {
    adjustStr();
    string_buf_ += matched();
  }
  \" {
    adjustStr();
    begin(StartCondition__::INITIAL);
    setMatched(string_buf_);
    string_buf_.clear();
    return Parser::STRING;
  }
  \\ {
    adjustStr();
    begin(StartCondition__::ESCAPE);
  }
}

<ESCAPE> {
  "n" {
    adjustStr();
    string_buf_ += "\n";
    begin(StartCondition__::STR);
  }
  
  "t" {
    adjustStr();
    string_buf_ += "\t";
    begin(StartCondition__::STR);
  }
  \" {
    adjustStr();
    string_buf_ += "\"";
    begin(StartCondition__::STR);
  }
  \\ {
    adjustStr();
    string_buf_ += "\\";
    begin(StartCondition__::STR);
  }
  [[:digit:]]{3} {
    adjustStr();
    string_buf_ += (char) atoi(matched().data());
    begin(StartCondition__::STR);
  }
  [[:space:]]+\\ {
    adjustStr();
    begin(StartCondition__::STR);
  }
  "^C" {
    adjustStr();
    string_buf_ += (char) 3;
    begin(StartCondition__::STR);
  }
  "^O" {
    adjustStr();
    string_buf_ += (char) 15;
    begin(StartCondition__::STR);
  }
  "^M" {
    adjustStr();
    string_buf_ += (char) 13;
    begin(StartCondition__::STR);
  }
  "^P" {
    adjustStr();
    string_buf_ += (char) 16;
    begin(StartCondition__::STR);
  }
  "^I" {
    adjustStr();
    string_buf_ += (char) 9;
    begin(StartCondition__::STR);
  }
  "^L" {
    adjustStr();
    string_buf_ += (char) 12;
    begin(StartCondition__::STR);
  }
  "^E" {
    adjustStr();
    string_buf_ += (char) 5;
    begin(StartCondition__::STR);
  }
  "^R" {
    adjustStr();
    string_buf_ += (char) 18;
    begin(StartCondition__::STR);
  }
  "^S" {
    adjustStr();
    string_buf_ += (char) 19;
    begin(StartCondition__::STR);
  }
}


"/*" {
  adjust();
  annotation_num++;
  begin(StartCondition__::COMMENT);
}
<COMMENT> {
  "/*" {
    adjust();
    annotation_num++;
  }
  "*/" {
    adjust();
    annotation_num--;
    if (annotation_num == 0)
      begin(StartCondition__::INITIAL);
  }

  .|\n adjust();
}


 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ {adjust();}
\n {adjust(); errormsg_->Newline();}


 /* illegal input */
. {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}
