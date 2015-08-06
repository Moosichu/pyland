import code
import sys
import io
import contextlib
import traceback

""" TODO: CLEAN THIS UP, THIS UP A LOT (do the same in bootsrapper.py), this is a really really really
weird, and as far as I can see, unnesecary way of defining a class. But what do I know :P
TODO: Make it so that the scoped_intepreter in bootstrapper.py is pulled from here.

Creates and defines a class called ScopedIntepreter which is used to run the player-code.
"""
class ScopedInterpreter(code.InteractiveInterpreter):

    output_reader = None

    """ Creates a scoped intepreter which can be used to run some code which is provided as the argument to runcode()
    imbued_locals -- A dictionary from strings to objects, each object will be available to the code being run in the interpreter under the name given by their dictionary key
    output_reader -- A function which takes a string as an argument, any output from the scoped code will be passed to here: TODO: work out if this is what it actually does!
    """
    def __init__(self, imbued_locals, output_reader):
        super().__init__(imbued_locals)
        self.output_reader = output_reader

    def runcode(self, code, HaltScriptException):
        #"""TODO: work out how to catch exceptions safely and handle them with python,
        #then we can query the database to print output and errors messages cleanly to the user!"""
        #TODO: work out what the hell contextlib.closing does, don't forget to change bootstrapper.py based on that!
        with contextlib.closing(sys.stdout):
            # Emulate super().runcode(code), but don't catch
            # RESTART, STOP or KILL because those should
            # propogate to allow thread control.
            #
            # The code here is a trivial modification of the
            # source of the implementaion we're overloading.
            try:
                exec(code, self.locals)
            except HaltScriptException as He:
                raise He
            except Exception as e:
                #Print out any errors caught in the code to the game terminal
                self.output_reader(traceback.format_exc(), True) 

