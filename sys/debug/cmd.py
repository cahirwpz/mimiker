import gdb
import traceback


def print_exception(func):
    def wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except Exception:
            traceback.print_exc()
    return wrapper


class AutoCompleteMixin():
    def options(self):
        raise NotImplementedError

    def complete_rest(self, first, text, word):
        return []

    # XXX: Completion does not play well when there are hypens in keywords.
    def complete(self, text, word):
        options = list(self.options())
        try:
            first, rest = text.split(' ', 1)
        except ValueError:
            return [option for option in options
                    if option.startswith(text) and text != option]
        return self.complete_rest(first, rest, word)


class SimpleCommand(gdb.Command):
    def __init__(self, name):
        super().__init__(name, gdb.COMMAND_USER)

    @print_exception
    def invoke(self, args, from_tty):
        return self(args)


class UserCommand():
    def __init__(self, name):
        self.name = name

    def __call__(self, args):
        raise NotImplementedError


class TraceCommand(UserCommand):
    def __init__(self, name, breakpoint):
        super().__init__(name)
        self.__instance = None
        self.breakpoint = breakpoint

    def __call__(self, args):
        if self.__instance:
            print('{} deactivated!'.format(self.__doc__))
            self.__instance.delete()
            self.__instance = None
        else:
            print('{} activated!'.format(self.__doc__))
            self.__instance = self.breakpoint()


class CommandDispatcher(SimpleCommand, AutoCompleteMixin):
    def __init__(self, name, commands):
        assert all(isinstance(cmd, UserCommand) for cmd in commands)

        self.name = name
        self.commands = {cmd.name: cmd for cmd in commands}
        super().__init__(name)

    def list_commands(self):
        return '\n'.join('{} {} -- {}'.format(self.name, name, cmd.__doc__)
                         for name, cmd in sorted(self.commands.items()))

    def __call__(self, args):
        try:
            cmd, args = args.split(' ', 1)
        except ValueError:
            cmd, args = args, ''

        if not cmd:
            print('{}\n\nList of commands:\n\n{}'.format(
                self.__doc__, self.list_commands()))
            return

        if cmd not in self.commands:
            raise gdb.GdbError('No such subcommand "{}"'.format(cmd))

        try:
            self.commands[cmd](args)
        except Exception:
            traceback.print_exc()

    def options(self):
        return list(self.commands.keys())
