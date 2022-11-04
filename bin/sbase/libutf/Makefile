AWK = awk
UNICODE = http://unicode.org/Public/UCD/latest/ucd/UnicodeData.txt

default:
	@echo Downloading and parsing $(UNICODE)
	@curl -\# $(UNICODE) | $(AWK) -f mkrunetype.awk
