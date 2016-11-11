def collect_values(tq, field):
    tq_it = tq['tqh_first']
    values = []
    while tq_it != 0:
        tq_it = tq_it.dereference()
        values.append(tq_it)
        tq_it = tq_it[field]['tqe_next']
    return values
