import gdb


class OneArgAutoCompleteMixin():
    def options(self):
        raise NotImplementedError

    def complete(self, text, word):
        args = text.split()
        options = self.options()
        if len(args) == 0:
            return options
        if len(args) >= 2:
            return []
        suggestions = []
        for o in options:
            if o.startswith(args[0], 0, len(o) - 1):
                suggestions.append(o)
        return suggestions


class GdbValueMixin(object):
    def to_string(self):
        return str(self)

    def display_hint(self):
        return 'map'


class GdbValueMeta(type):
    def __new__(cls, name, bases, dct):
        t = gdb.lookup_type(dct['__ctype__'])
        # for each field of ctype make property getter of same name
        for f in t.fields():
            def mkgetter(fname, caster):
                if caster is None:
                    return lambda x: x._obj[fname]
                return lambda x: caster(x._obj[fname])
            caster = None
            if '__cast__' in dct:
                caster = dct['__cast__'].get(f.name, None)
            dct[f.name] = property(mkgetter(f.name, caster))
        return super(GdbValueMeta, cls).__new__(
                cls, name, (GdbValueMixin,) + bases, dct)
