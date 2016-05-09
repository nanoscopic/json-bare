json-bare
========

Extremely fast JSON parser with full tree in C.

The parser is based on a basic state machine, using the minimal number of states to parse JSON
rapidly. The internal node structure created is equal to that used in the XML parser xml-bare.
This is currently intentional and allows JSON to be used as if it was XML.


Known Issues:
* I don't believe the parser currently cleans up memory correctly.
* The way hash storage is done currently there is no way iterate through hash members.
* No serialization is currently implemented
* Values are currently stored as string regardless of whether the actual value is a string or a number
* Numbers are not currently parsed into a number; they remain a string
