# lab2-doc

## 处理Comment

- /*: 第一次遇到后annotation_num++，进入<COMMENT>状态，并adjust

​			 之后遇到annotation_num++，并adjust

- */: 遇到后annotation_num--, adjust

​			 若annotation_num在做减操作后等于零，则返回默认状态<INITIAL>

- 其它字符|\n: adjust



## 处理String

- ": 第一次遇到后进入<STR>状态，adjust

​		   下一次遇到adjustStr，并将所匹配的字符串setMatched，返回默认状态<INITIAL>,返回Parser::STRING

- \：进入状态<ESCAPE>（可能是转义字符，也可能是某些特殊字符）

  <ESCAPE>中遇到"n","t",",\: 加入到string_buf_中，adjustStr，回到状态<STR>中

  <ESCAPE>中遇到[[:digit:]]{3}：将对应ASCII编码值的字符加入到string_buf_中，adjustStr，回到状态<STR>

  <ESCAPE>中遇到[[:space:]]+\\：忽略空白字符，adjustStr

  <ESCAPE>中遇到转义字符：将对应的ASCII码值(int)强制类型转换成char，adjustStr，回到状态<STR>

- 除了\"和\\之外的可打印字符和空白字符: 将匹配的字符串加入到string_buf_中，adjustStr



## 处理错误

- 如果不匹配前面所写的所有regular expressions，则打印错误



## 文件终止符处理

- 不做任何处理