#include "sudohc.h"

std::vector<Token> tokenize(const std::string& line)
{
	// symbols which act as both delimiters and tokens themselves
	static const std::vector<std::string> delimTokens = {
		" ", ",", "\"", "\n", "\t",
		"(", ")", "[", "]", "{", "}",
		"<-", "+", "-", "*", "//", "/",
		"<=", "<", ">=", ">", "=", "!="
	};

	std::vector<Token> tokens;

	int idxBeginToken = 0;
	bool beginLine = true;
	int lineNum = 1;
	for (int curr = 0; curr < line.length(); curr++)
	{
		// check if the token at index is a delimiter
		const std::string* delim = nullptr;
		for (const std::string& d : delimTokens)
		{
			if (line.compare(curr, d.length(), d) == 0)
			{
				delim = &d;
				break;
			}
		}

		// if token at index was not delimiter then skip
		if (!delim)
		{
			continue;
		}

		// add new token
		if (curr > idxBeginToken)
		{
			tokens.push_back({ lineNum, curr, line.substr(idxBeginToken, curr - idxBeginToken) });
		}



		// add delimiter as a token as well (if it is not space)
		if (*delim != " ")
		{
			// if delimeter is a string then close the string if it is valid
			if (*delim == "\"")
			{
				// try to find closing quote
				size_t close;
				for (close = curr + 1; close < line.length(); close++)
				{
					if (line[close] == '\"' && line[close - 1] != '\\')
					{
						break;
					}
				}
				// if loop iterated through rest of string but still
				// didnt find closing quote then string is mal-formed
				if (close == line.length())
				{
					throw SyntaxException("malformed string");
				}

				tokens.push_back({ lineNum, curr, line.substr(curr, close - curr + 1) });
				curr = close;
			}
			else if (*delim == "//")
			{
				size_t end = line.find('\n', curr);
				tokens.push_back({ lineNum, curr, line.substr(curr, end) });
				curr = end;
			}
			else
			{
				beginLine = *delim == "\n" ? lineNum++, true : (*delim == "\t" && beginLine);

				// do not add tabs after the beginning of a line to the list
				bool tabAfterBegin = *delim == "\t" && !beginLine;
				if (!tabAfterBegin)
				{
					tokens.push_back({ lineNum, curr, *delim });
				}

				curr += delim->length() - 1;
			}
		}

		idxBeginToken = curr + 1;
	}

	return tokens;
}