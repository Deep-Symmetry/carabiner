import socket
import edn_format

class LinkListener():
    """A simple python client to communicate with carabiner (a Abelton Link connector)
    Carabiner server must be running to use. Requires edn_format [$pip install edn_format]"""
    def __init__(self, tcp_ip = '127.0.0.1', tcp_port = 17000, buffer_size = 1024):
        self.tcp_ip = tcp_ip
        self.tcp_port = tcp_port
        self.buffer_size = buffer_size
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.connect((tcp_ip, tcp_port))

    def decode_edn_msg(self, msg):
        """Decodes a TCP message from Carabiner to python dictionary"""
        msg = msg.decode()
        striped_msg = msg[msg.index('{'):]
        decoded_msg = edn_format.loads(striped_msg)

        # Because the edf_format package does not return normal dam dicts (or string keywords). What dicts.
        if type(decoded_msg) is edn_format.immutable_dict.ImmutableDict:
            decoded_dict = {}
            for key, value in decoded_msg.dict.items():
                print()
                decoded_dict[str(key).strip(':')] = value
            decoded_msg = decoded_dict

        return decoded_msg

    def status(self):
        """Wrapper for Status"""
        self.s.send(b'status')
        return_msg = self.s.recv(self.buffer_size)
        return self.decode_edn_msg(return_msg)

    def bpm(self, bpm):
        """Wrapper for Beat At Time"""
        msg = 'bpm ' + str(bpm)
        self.s.send(msg.encode())
        return_msg = self.s.recv(self.buffer_size)
        return self.decode_edn_msg(return_msg)

    def beat_at_time(self, time_in_nsecs, quantum=4):
        """Wrapper for Beat At Time"""
        msg = 'beat-at-time ' + str(time_in_nsecs) + ' ' + str(quantum)
        self.s.send(msg.encode())
        return_msg = self.s.recv(self.buffer_size)
        return self.decode_edn_msg(return_msg)


    def force_beat_at_time(self, beat, time_in_nsecs, quantum=4):
        """Wrapper for Beat At Time"""
        msg = 'force-beat-at-time ' + str(beat) + ' ' + str(time_in_nsecs) + ' ' + str(quantum)
        self.s.send(msg.encode())
        return_msg = self.s.recv(self.buffer_size)
        return self.decode_edn_msg(return_msg)

    def __del__(self):
        self.s.close()


if __name__ == "__main__":
    link = LinkListener()
    status = link.status()
    print(link.force_beat_at_time(12, status['start']+10000))
