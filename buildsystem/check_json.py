import json
import sys

def check_color(color):
    l = len(color)
    if l != 4 and l != 7 and l != 9:
        raise ValueError('error: invalid color length ' + color)
    for c in color[1:]:
        if not c.isalnum():
            raise ValueError('error: invalid color hex ' + color)
    return;

def id_generator(dict_var):
      for k, v in dict_var.items():
        if isinstance(v, dict):
            for id_val in id_generator(v):
                yield id_val
        elif isinstance(v, str) and v.startswith('#'):
            yield v            

def is_valid(json_path):
    try:
        with open(json_path) as f:
            json_object = json.load(f)
            for _ in id_generator(json_object):
                check_color(_)
            
    except ValueError as e:
        print(e)
        return False
    return True

if len(sys.argv) == 1:
    print("ERROR: no json given!")
    sys.exit(-1)

if is_valid(sys.argv[1]):
    print("themes JSON is OK")    
    sys.exit(0)
else:
    print("ERROR: themes JSON INVALID")
    sys.exit(1)