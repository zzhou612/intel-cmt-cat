################################################################################
# BSD LICENSE
#
# Copyright(c) 2019-2022 Intel Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#   * Neither the name of Intel Corporation nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
################################################################################

import argparse

from pqos import Pqos
from pqos.cpuinfo import PqosCpuInfo
from pqos.l3ca import PqosCatL3


def str_to_int(num_str):
    """
    Converts string into number.

    Parameters:
        num_str: a string to be converted into number

    Returns:
        numeric value of the string representing the number
    """

    if num_str.lower().startswith('0x'):
        return int(num_str[2:], 16)

    return int(num_str)

# /**
#  * @brief Verifies and translates definition of single
#  *        allocation class of service
#  *        from args into internal configuration.
#  *
#  * @param argc Number of arguments in input command
#  * @param argv Input arguments for COS allocation
#  */

def set_allocation_class(sockets, class_id, mask):
    """
    Sets up allocation classes of service on selected CPU sockets

    Parameters:
        sockets: a list of socket IDs
        class_id: class of service ID
        mask: COS bitmask
    """

    l3ca = PqosCatL3()
    cos = l3ca.COS(class_id, mask)

    for socket in sockets:
        try:
            l3ca.set(socket, [cos])
        except:
            print("Setting up cache allocation class of service failed!")


def print_allocation_config(sockets):
    """
    Prints allocation configuration.

    Parameters:
        sockets: a list of socket IDs
    """

    l3ca = PqosCatL3()

    for socket in sockets:
        try:
            coses = l3ca.get(socket)

            print("L3CA COS definitions for Socket %u:" % socket)

            for cos in coses:
                cos_params = (cos.class_id, cos.mask)
                print("    L3CA COS%u => MASK 0x%x" % cos_params)
        except:
            print("Error")
            raise


def parse_args():
    """
    Parses command line arguments.

    Returns:
        an object with parsed command line arguments
    """

    def int_dec_or_hex(num):
        try:
            return str_to_int(num)
        except:
            msg = 'Cannot convert a parameter to a number'
            raise argparse.ArgumentError(msg)

    description = 'PQoS Library Python wrapper - L3 CAT allocation example'
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('-I', dest='interface', action='store_const',
                        const='OS', default='MSR',
                        help='select library OS interface')
    parser.add_argument('class_id', type=int, help='COS ID')
    parser.add_argument('mask', type=int_dec_or_hex, help='COS bitmask')

    args = parser.parse_args()
    return args


class PqosContextManager:
    """
    Helper class for using PQoS library Python wrapper as a context manager
    (in a with statement).
    """

    def __init__(self, *args, **kwargs):
        self.args = args
        self.kwargs = kwargs
        self.pqos = Pqos()

    def __enter__(self):
        "Initializes PQoS library."

        self.pqos.init(*self.args, **self.kwargs)
        return self.pqos

    def __exit__(self, *args, **kwargs):
        "Finalizes PQoS library."

        self.pqos.fini()
        return None


def main():
    args = parse_args()

    try:
        with PqosContextManager(args.interface):
            cpu = PqosCpuInfo()
            sockets = cpu.get_sockets()

            set_allocation_class(sockets, args.class_id, args.mask)
            print_allocation_config(sockets)
    except:
        print("Error!")
        raise


if __name__ == "__main__":
    main()
