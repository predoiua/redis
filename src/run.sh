#!/bin/sh
java -classpath %CLASSPATH%:antlr-3.3-complete.jar org.antlr.Tool VVExpr.g
java -classpath %CLASSPATH%:antlr-3.3-complete.jar org.antlr.Tool VVEval.g
java -classpath %CLASSPATH%:antlr-3.3-complete.jar org.antlr.Tool VVEvalInit.g
