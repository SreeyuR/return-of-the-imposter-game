import sys
import re

in_paths = sys.argv[1:-1]
out_path = sys.argv[-1]

# Dict that maps symbol names to values
symbols = {}

# Stack of [min_x, min_y, max_x, max_y] of frames
frame_stack = []

# Stack of Boolean values associated with if statements
if_stack = []

# Map from function name to (arg_names, [line1, line2, ...])
func_dict = {}
func_body_active = None

keywords = [
    "wall",
    "player",
    "key",
    "door",
    "vent",
    "trampoline",
    "damaging_obstacle",
    "crewmate",
    "decoration",
    "rect",
    "star",
    "--",
    "-=",
    "-+",
    "=-",
    "==",
    "=+",
    "+-",
    "+=",
    "++",
]


def convert_coords_to_frame(x, y):
    if len(frame_stack) > 0:
        return (x + frame_stack[-1][0], y + frame_stack[-1][1])
    return (x, y)


def set_frame_symbols():
    if len(frame_stack) == 0:
        return
    min_x, min_y, max_x, max_y = frame_stack[-1]
    symbols["@FW"] = max_x - min_x
    symbols["@FH"] = max_y - min_y


def evaluate_expr(expr):
    if expr in keywords or expr in func_dict:
        return expr
    if expr[0] == "{" and expr[-1] == "}":
        return [
            evaluate_expr(element)
            for element in re.findall(r"[^\s]+|\{.*\}", expr[1:-1])
        ]
    return eval(expr)


def parse_shape(pos, shape):
    anchor, x, y = pos
    x, y = convert_coords_to_frame(x, y)
    shape_type = shape[0]
    shape_args = [anchor, x, y] + shape[1:]
    if shape_type == "rect":
        width, height = map(float, shape[1:])
        assert width >= 0 and height >= 0
    return "{" + shape_type + "," + ",".join(map(str, shape_args)) + "}"


def substitute_values(expr):
    return re.sub(
        r"[a-zA-Z0-9_@]+",
        lambda s: str(symbols[s.group(0)]) if s.group(0) in symbols else s.group(0),
        expr
    )


def parse_line(line, outfile):
    global symbols, func_body_active
    line = line.strip()
    # Remove comments
    line = re.sub(r"#.*$", "", line)
    # Empty line
    if line == "":
        return
    command = line.split()[0]
    # Function declaration
    if command == "func":
        args = line.split()[1:]
        func_name = args[0]
        func_body_active = func_name
        func_dict[func_name] = (args[1:], [])
        return
    elif func_body_active:
        if command == "endfunc":
            func_body_active = None
        else:
            func_dict[func_body_active][1].append(line)
        return
    elif command == "endif":
        if_stack.pop()
        return
    elif command == "else":
        if_stack[-1] = not if_stack[-1]
        return
    # Currently inside a false if statement
    if False in if_stack:
        # Nested if statement inside false if block - need to keep track
        # so it is correctly popped by its endif
        if command == "if":
            if_stack.append(False)
        return
    # Symbol declaration
    if command == "let":
        name, value = line.split(" ", 2)[1:]
        symbols[name] = eval(substitute_values(value))
        return
    # Substitute symbol values for names
    line = substitute_values(line)
    # Tokens are groups of non-space characters, or any characters
    # enclosed by curly brackets.
    tokens = re.findall(r"(?:[^\{\}\s]+=)?(?:\{.*?\}|[^\s]+)", line)
    # Empty line
    if len(tokens) == 0:
        return
    args = tokens[1:]
    # If statement
    if command == "if":
        if_stack.append(eval(" ".join(args)))
        return
    args_dict = {}
    unnamed_args = []
    # Evaluate expressions in args
    for arg in args:
        arg_split = arg.split("=", 1)
        if len(arg_split) == 2:
            arg_name, arg_value = arg_split
            args_dict[arg_name] = evaluate_expr(arg_value)
        else:
            unnamed_args.append(evaluate_expr(arg))
    # Call a function
    if command == "call":
        func_name = unnamed_args[0]
        # Create symbols for function arguments
        for i, func_arg in enumerate(unnamed_args[1:]):
            symbols[func_dict[func_name][0][i]] = func_arg
        # Parse each line of the function body
        for func_line in func_dict[func_name][1]:
            try:
                parse_line(func_line, outfile)
            except:
                print(func_line.strip())
                raise
        return
    # Frame creation
    elif command == "frame":
        min_x, min_y, max_x, max_y = map(float, unnamed_args)
        min_x, min_y = convert_coords_to_frame(min_x, min_y)
        max_x, max_y = convert_coords_to_frame(max_x, max_y)
        frame_stack.append((min_x, min_y, max_x, max_y))
        set_frame_symbols()
    # Frame end
    elif command == "endframe":
        frame_stack.pop()
        set_frame_symbols()
    # Body creation
    elif command == "body":
        out_line = command
        pos = args_dict["pos"]
        shape = args_dict["shape"]
        traj_pos = args_dict["trajectory_pos"] if "trajectory_pos" in args_dict else pos
        traj_shape = args_dict["trajectory_shape"] if "trajectory_shape" in args_dict else None
        out_line += " " + "shape=" + parse_shape(pos, shape)
        if traj_shape:
            out_line += " " + "trajectory_shape=" + parse_shape(traj_pos, traj_shape)
        for arg in args_dict:
            if arg in ("pos", "shape", "trajectory_pos", "trajectory_shape"):
                continue
            out_line += " " + arg + "="
            if arg == "color":
                out_line += hex(args_dict[arg])
            else:
                out_line += str(args_dict[arg])
        outfile.write(out_line + "\n")
    elif command == "scene_boundary":
        min_x, min_y, max_x, max_y = map(float, unnamed_args)
        min_x, min_y = convert_coords_to_frame(min_x, min_y)
        max_x, max_y = convert_coords_to_frame(max_x, max_y)
        out_line = command + (" %f %f %f %f" % (min_x, min_y, max_x, max_y))
        outfile.write(out_line + "\n")
    elif command == "print":
        print(args)
    elif command == "error":
        raise RuntimeError()
    else:
        raise ValueError("%s is not a valid command" % command)


for in_path in in_paths:
    with open(in_path, "r") as infile:
        with open(out_path, "w") as outfile:
            for line in infile:
                try:
                    parse_line(line, outfile)
                except:
                    print(line.strip())
                    raise

assert len(frame_stack) == 0 and len(if_stack) == 0 and not func_body_active
