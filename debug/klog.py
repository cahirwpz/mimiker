import gdb
from ptable import ptable, as_hex


class KernelLog():
    def invoke(self):
        messages, klog = self.load_klog()
        self.dump_general_info(len(messages), klog)
        self.dump_kernel_logs(messages)

    def format_message(self, data):
        message = data['kl_format'].string()
        arguments = data['kl_params']
        # If there is % escaped it is not parameter.
        number_of_parameters = message.count('%') - 2 * message.count('%%')
        params = ', '.join(str(arguments[i])
                           for i in range(number_of_parameters))
        try:
            message = message.replace('"', '\\"')
            # Using gdb printf so we don't need to dereference addresses.
            formated = gdb.execute(
                'printf "' + message + ' ", ' + params, to_string=True)
        except Exception:
            print('''Error in formating message "{}" with parameters "{}"\n
            Message skipped!!'''.format(message, params))
            formated = message + params
        time = "%d.%06d" % (data['kl_timestamp']['tv_sec'],
                            data['kl_timestamp']['tv_usec'])
        return [time, "%s:%d" % (data['kl_file'].string(), data['kl_line']),
                str(data['kl_origin']), str(formated)]

    def load_klog(self):
        klog = gdb.parse_and_eval('klog')
        first = int(klog['first'])
        last = int(klog['last'])
        klog_array = klog['array']
        array_size = int(klog_array.type.range()[1]) + 1
        messages = []
        while first != last:
            messages.append(self.format_message(klog_array[first]))
            first = (first + 1) % array_size
        return messages, klog

    def dump_general_info(self, number_of_logs, klog):
        rows_general = [["Mask", "Verbose", "Log number"]]
        mask = as_hex(klog['mask'])
        verbose = bool(klog['verbose'])
        rows_general.append([mask, str(verbose), str(number_of_logs)])
        ptable(rows_general, header=True, fmt='l')

    def dump_kernel_logs(self, messages):
        rows_data = [["Time", "Source", "System", "Message"]]
        rows_data.extend(messages)
        ptable(rows_data, header=True, fmt='rrrl')
