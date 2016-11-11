def collect_values(tq, field):
    tq_it = tq['tqh_first']
    ret = []
    while tq_it != 0:
        tq_it = tq_it.dereference() #tq_it is now value
        ret.append(tq_it)
        tq_it = tq_it[field]['tqe_next']
    return ret

