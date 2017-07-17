class TailQueue():
    def __init__(self, tq, field):
        self.tq = tq
        self.field = field

    def __iter__(self):
        item = self.tq['tqh_first']
        while item != 0:
            item = item.dereference()
            yield item
            item = item[self.field]['tqe_next']
