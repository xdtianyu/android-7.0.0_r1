import asyncore
import btsocket
import struct
import logging


BT_ATT_DEFAULT_LE_MTU = 23

BT_ATT_OP_MTU_REQ = 0x02
BT_ATT_OP_MTU_RSP = 0x03


class BluetoothGATTServerError(Exception):
    """Error raised for GATT-related issues with BluetoothGATTServer."""
    pass


class BluetoothGATTServer(asyncore.dispatcher):
    """Bluetooth GATT Server.

    This class inherits asyncore.dispatcher to handle I/O on BLE socket.
    It is essentially a socket service server. The class implements
    initialization, read/write requests, notifications and indications.

    Before creating an instance of this class, the machine must be set up
    (with appropriate HCI commands) to be at least advertising, be powered,
    and have LE turned on.

    """

    def __init__(self, mtu=BT_ATT_DEFAULT_LE_MTU):
        self.mtu = max(mtu, BT_ATT_DEFAULT_LE_MTU)

        sock, addr = btsocket.create_le_gatt_server_socket()
        logging.debug('incoming connection from address %s', addr)

        asyncore.dispatcher.__init__(self, sock=sock)
        asyncore.loop()


    def exchange_mtu(self, data):
        """Handle exchange MTU request.

        Exchange MTU request/response usually initiates client-server
        communication. The method sends exchange MTU response back to client.
        It also sets value for MTU attribute.

        @param data: Raw data received from socket (without opcode).

        """
        if len(data) != 2:
            raise BluetoothGATTServerError(
                'Invalid MTU size: expected 2 bytes for Exchange MTU Request')

        client_rx_mtu = struct.unpack('<H', data)
        if client_rx_mtu < BT_ATT_DEFAULT_LE_MTU:
            raise BluetoothGATTServerError('Invalid MTU size: %d < %d' %
                                           client_rx_mtu, BT_ATT_DEFAULT_LE_MTU)

        self.mtu = min(client_rx_mtu, self.mtu)

        response = struct.pack('<BH', BT_ATT_OP_MTU_RSP, self.mtu)
        self.send(response)


    def handle_read(self):
        """Receive and handle a single message from the socket.

        This method gets called when the asynchronous loop detects that a read()
        call on the channel's socket will succeed. It overrides handle_read()
        from asyncore.dispatcher class.

        """
        data = self.recv(self.mtu)

        opcode = ord(data[0])
        data = data[1:]

        func_map = {BT_ATT_OP_MTU_REQ: self.exchange_mtu}
        if opcode not in func_map:
            raise BluetoothGATTServerError('Invalid Opcode: %d' % opcode)

        func_map[opcode](data)
