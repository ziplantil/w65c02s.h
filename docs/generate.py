#!/usr/bin/python3
import re
import textwrap

def convert(file, text, signature):
    text = text.strip()
    assert text.startswith("/**")
    text = text[3:].lstrip()
    assert text.endswith("*/")
    text = text[:-2]
    text = re.sub(r"(?m)^\s*\*[ \t]{,2}", "", text)

    text = re.sub(r"\n(?![\[\n])", " ", text)
    text = re.sub(r" +", " ", text)
    text = re.sub(r"(?m)^ +", "", text)

    name, text = text.split("\n", 1)
    signature = signature.strip()
    brief, text = text.split("\n", 1)
    desc = ""

    params = []
    retval = None
    
    for line in text.splitlines():
        if line.startswith("[") and "]" in line:
            tag, line = line[1:].split("]", 1)
            tag, line = tag.strip(), line.strip()
            if tag.startswith("Parameter:"):
                param = tag.split(":")[1].strip()
                params.append((param, line))
            elif tag == "Return value":
                retval = line
        else:
            desc += line + "\n"
    
    desc = desc.rstrip()
    print("## {}".format(name), file = file)
    print("\n".join(textwrap.wrap(brief, 79)), file = file)
    print("", file = file)
    print("```c\n{}\n```".format(signature), file = file)
    print("", file = file)
    for desc_line in desc.splitlines():
        print("\n".join(textwrap.wrap(desc_line, 79)), file = file)
        print("", file = file)
    for param, paramdesc in params:
        print("\n".join(textwrap.wrap(paramdesc, 79,
            initial_indent = "* **Parameter** `{}`: ".format(param),
            subsequent_indent="  ")), file = file)
    if retval:
        print("\n".join(textwrap.wrap(retval, 79,
            initial_indent = "* **Return value**: ",
            subsequent_indent="  ")), file = file)
    print("", file = file)

with open("../src/w65c02s.h", "r") as source_file:
    source = source_file.read()

with open("api.md", "w") as target:
    print("""```c
#include "w65c02s.h"
```
""", file = target)
    for comment_block, signature in re.findall(r"(?s)(/\*\* .+?\*/)([^;]*?;)", source):
        comment_block = re.sub(r"(W65C02SCE_[A-Z0-9_]+)", r"`\1`", comment_block)
        convert(target, comment_block, signature)
