class Function:
    def __init__(self, expression=None, argument_names=None):
        if argument_names is None:
            argument_names = []
        self.expression = expression
        self.argument_names = argument_names


class ScriptFunction:
    def __init__(self, callback=None, enable_arguments=False):
        self.callback = callback
        self.enable_arguments = enable_arguments


class SymbolTable:
    def __init__(self, variables=None, constants=None, functions=None, scripts=None):
        if variables is None:
            variables = {}
        if constants is None:
            constants = {}
        if functions is None:
            functions = {}
        if scripts is None:
            scripts = {}
        self.variables = variables
        self.constants = constants
        self.functions = functions
        self.scripts = scripts

    def remove(self, name):
        self.variables.pop(name, None)
        self.constants.pop(name, None)
        self.functions.pop(name, None)
        self.scripts.pop(name, None)

    def get_variable_names(self):
        return self.variables.keys()

    def get_variable(self, name):
        return self.variables[name]

    def set_variable(self, name, value):
        self.variables[name] = value

    def get_constant_names(self):
        return self.constants.keys()

    def get_constant(self, name):
        return self.constants[name]

    def set_constant(self, name, value):
        self.constants[name] = value

    def get_function_names(self):
        return self.functions.keys()

    def get_function(self, name):
        return self.functions[name]

    def set_function(self, name, func):
        self.functions[name] = func

    def get_script_names(self):
        return self.scripts.keys()

    def get_script(self, name):
        return self.scripts[name]

    def set_script(self, name, script):
        self.scripts[name] = script

    def set_script_noargs(self, name, callback):
        self.scripts[name] = ScriptFunction(callback, False)