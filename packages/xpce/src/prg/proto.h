
/* ../src/prg/operator.c */
status		makeClassOperator(Class class);

/* ../src/prg/parser.c */
status		makeClassParser(Class class);

/* ../src/prg/tokeniser.c */
Tokeniser	getOpenTokeniser(Tokeniser t, Any source);
Int		getPeekTokeniser(Tokeniser t);
status		symbolTokeniser(Tokeniser t, Name symb);
status		makeClassTokeniser(Class class);
