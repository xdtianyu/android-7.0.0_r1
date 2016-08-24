#!/bin/env python
#
# Copyright (C) 2014 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging
import os
import re
import sys
import tempfile
import textwrap
import uuid

from xml.etree import ElementTree
from xml.dom import minidom

# We are dealing with unicode data. It is extremely important to choose between
# the |unicode| type and the |str| type with unicode encoding as the default
# storage type for strings, and stick to it.
# - All strings except filenames and such are of type |unicode|
#   - Note that the xml.etree.ElementTree.parse function actually returns
#     strings in the |str| type. These will be implicitly coerced to |unicode|
#     as needed. If you don't like this, add a phase to explicitly cast these
#     strings.
# - Whenever using the |str| type, use the suffix |_str|
# - Moreover, whenever using |str| type with |ascii| encoding, using
#   |_str_ascii| suffix
FILE_ENCODING = 'utf-8'

class ConverterError(Exception):
    pass


class ServiceProvidersConverter(object):
    """ Convert the ServiceProviders XML into protobuf format. """
    def __init__(self, file_path, out_file_path=None):
        """
        @param file_path: Absolute path to the XML file to read
        @param out_file_path: Absolute path to the file to which the output
                should be written.

        """
        self._file_path = file_path
        self._out_file_path = out_file_path

        self._gsm_nodes_no_mccmnc = set()
        self._gsm_nodes_by_mccmnc = {}
        self._mcc_mnc_by_mccmnc = {}

        # Book-keeping to sanity check the total number of providers converted,
        # and detailed information about the conversion.
        self._xml_cdma_nodes = 0
        self._xml_gsm_nodes = 0
        self._protobuf_mnos_dumped = 0
        self._protobuf_mvnos_dumped = 0
        self._protobuf_gsm_mnos = 0
        self._protobuf_cdma_mnos = 0
        self._protobuf_gsm_mvnos = 0
        self._protobuf_gsm_unique_mvnos = 0
        # Turns out some MVNOs are MNOs using a different MCCMNC.
        self._protobuf_gsm_mvnos_mnos = 0
        # Remember nodes that we decide to drop at any point.
        self._dropped_nodes = set()

        # Related to the actual protobuf output:
        self._indent = 0


    def Convert(self):
        """ Top level function for the conversion. """
        parser = ElementTree.XMLParser(encoding=FILE_ENCODING)
        element_tree = ElementTree.parse(self._file_path, parser=parser)
        self._root = element_tree.getroot()
        logging.info('Dumping parsed XML')
        self._DumpXMLToTempFile()
        self._xml_cdma_nodes = len(self._root.findall(u'.//cdma'))
        self._xml_gsm_nodes = len(self._root.findall(u'.//gsm'))

        self._TransformXML()
        logging.info('Dumping transformed XML.')
        self._DumpXMLToTempFile()

        self._GroupGSMNodesByMCCMNC()
        self._FindPrimaryNodes()

        if self._out_file_path is not None:
            with open(self._out_file_path, 'w') as self._out_file:
                self._SpewProtobuf()
        else:
            self._out_file = sys.stdout
            self._SpewProtobuf()

        self._RunStatsDiagnostics()


    def _CheckStatsEqual(self, lhs, lhs_name, rhs, rhs_name):
        """
        Test that |lhs| == |rhs| and log appropriate message.

        @param lhs: One value to compare.
        @param lhs_name: str name to be used for |lhs| for logging.
        @param rhs: Other value to compare.
        @param rhs_name: str name to be used for |rhs| for logging.
        @return True if check passes, False otherwise.

        """
        result = (lhs == rhs)
        logger = logging.info if result else logging.error
        message = 'PASS' if result else 'FAIL'
        logger('Sanity check: (%s) == (%s) (%d == %d) **%s**',
               lhs_name, rhs_name, lhs, rhs, message)
        return result


    def _RunStatsDiagnostics(self):
        """ Checks that the stats about nodes found / dumped tally. """
        # First dump dropped nodes.
        if len(self._dropped_nodes) > 0:
            logging.warning('Following nodes were dropped:')
            for node in self._dropped_nodes:
                logging.info(self._PPrintXML(node).encode(FILE_ENCODING))

        logging.info('######################')
        logging.info('Conversion diagnostics')
        logging.info('######################')

        logging.info('Total number of XML CDMA nodes read [xml_cdma_nodes]: %d',
                     self._xml_cdma_nodes)
        logging.info('Total number of XML GSM nodes read [xml_gsm_nodes]: %d',
                     self._xml_gsm_nodes)
        logging.info('Total number of XML nodes read '
                     '[xml_nodes = xml_cdma_nodes + xml_gsm_nodes]: %d',
                     self._xml_cdma_nodes + self._xml_gsm_nodes)

        logging.info('Total number of protobuf MNOs dumped '
                     '[protobuf_mnos_dumped]: %d',
                     self._protobuf_mnos_dumped)
        logging.info('Total number of protobuf MVNOs dumped '
                     '[protobuf_mvnos_dumped]: %d',
                     self._protobuf_mvnos_dumped)
        logging.info('Total number of protobuf nodes dropped '
                     '[protobuf_dropped_nodes]: %d',
                     len(self._dropped_nodes))
        logging.info('  (See above for the exact nodes dropped)')

        logging.info('Total number of protobuf CDMA MNOs '
                     '[protobuf_cdma_mnos]: %d',
                     self._protobuf_cdma_mnos)
        logging.info('Total number of protobuf GSM MNOs '
                     '[protobuf_gsm_mnos]: %d',
                     self._protobuf_gsm_mnos)
        logging.info('Total number of protobuf GSM MVNOs '
                     '[protobuf_gsm_mvnos]: %d',
                     self._protobuf_gsm_mvnos)
        logging.info('Total number of protobuf unique GSM MVNOs. '
                     '[protobuf_gsm_unique_mvnos]: %d',
                     self._protobuf_gsm_unique_mvnos)
        logging.info('  (Some MVNOs may appear in multiple MNOs)')
        logging.info('Total number of protobuf GSM MVNOs that are also MNOs. '
                     '[protobuf_gsm_mvnos_mnos]: %d',
                     self._protobuf_gsm_mvnos_mnos)

        check_results = []
        check_results.append(self._CheckStatsEqual(
                self._protobuf_mnos_dumped,
                'protobuf_mnos_dumped',
                self._protobuf_cdma_mnos + self._protobuf_gsm_mnos,
                'protobuf_cdma_mnos + protobuf_gsm_mnos'))

        check_results.append(self._CheckStatsEqual(
                self._protobuf_mnos_dumped + self._protobuf_mvnos_dumped,
                'protobuf_mnos_dumped + protobuf_mvnos_dumped',
                (self._protobuf_cdma_mnos +
                 self._protobuf_gsm_mnos +
                 self._protobuf_gsm_mvnos),
                'protobuf_cdma_mnos + protobuf_gsm_mnos + protobuf_gsm_mvnos'))

        check_results.append(self._CheckStatsEqual(
                self._xml_cdma_nodes + self._xml_gsm_nodes,
                'xml_cdma_nodes + xml_gsm_nodes',
                (len(self._dropped_nodes) +
                 self._protobuf_gsm_mnos +
                 self._protobuf_cdma_mnos +
                 self._protobuf_gsm_unique_mvnos -
                 self._protobuf_gsm_mvnos_mnos),
                ('protobuf_dropped_nodes + '
                 'protobuf_gsm_mnos + protobuf_cdma_mnos + '
                 'protobuf_gsm_unique_mvnos - protobuf_gsm_mvnos_mnos')))

        if False in check_results:
            self._LogAndRaise('StatsDiagnostics failed.')


    def _DumpXMLToTempFile(self):
        """ Dumps the parsed XML to a temp file for debugging. """
        fd, fname = tempfile.mkstemp(prefix='converter_')
        logging.info('Dumping XML to file %s', fname)
        with os.fdopen(fd, 'w') as fout:
            fout.write(self._PPrintXML(self._root).encode(FILE_ENCODING))


    def _EnrichNode(self, node, country_code, primary, roaming_required, names,
                    provider_type):
        """
        Adds the information passed in as children of |node|.

        @param node: The XML node to enrich.
        @param country_code: The country code for node. Type: str.
        @param primary: Is this node a primary provider. Type: str
        @param roaming_required: Does this provider requires roaming. Type: str.
        @param names: List of names for this provider. Type: [(str, str)].
        @param provider_type: Is this node 'gsm'/'cdma'. Type: str.

        """
        ElementTree.SubElement(node, u'country', {u'code': country_code})
        provider_map = {}
        provider_map[u'type'] = provider_type
        if primary is not None:
            provider_map[u'primary'] = primary
        if roaming_required is not None:
            provider_map[u'roaming-required'] = roaming_required
        ElementTree.SubElement(node, u'provider', provider_map)
        for name, lang in names:
            name_map = {}
            if lang is not None:
                name_map[u'xml:lang'] = lang
            name_node = ElementTree.SubElement(node, u'name', name_map)
            name_node.text = name


    def _TransformXML(self):
        """
        Store the country, provider, name, type (gsm/cdma) under the
        |gsm|/|cdma| nodes. This allows us to directly deal with these nodes
        instead of going down the tree.

        """
        # First find all nodes to be modified, since we can't iterate the tree
        # while modifying it.
        nodes = {}
        for country_node in self._root.findall(u'country'):
            cur_country = country_node.get(u'code')
            for provider_node in country_node.findall(u'provider'):
                primary = provider_node.get(u'primary')
                roaming_required = provider_node.get(u'roaming-required')
                names = [(name_node.text, name_node.get(u'xml:lang')) for
                         name_node in provider_node.findall(u'name')]

                for gsm_node in provider_node.findall(u'gsm'):
                    nodes[gsm_node] = (cur_country,
                                       primary,
                                       roaming_required,
                                       names,
                                       u'gsm')
                for cdma_node in provider_node.findall(u'cdma'):
                    # Some CDMA providers have a special name under the <cdma>
                    # node. This name should *override* the names given outside.
                    if cdma_node.find(u'name') is not None:
                        names = []
                    nodes[cdma_node] = (cur_country,
                                        primary,
                                        roaming_required,
                                        names,
                                        u'cdma')

        # Now, iterate through all those nodes and update the tree.
        for node, args in nodes.iteritems():
            self._EnrichNode(node, *args)


    def _CheckAmbiguousMCCMNC(self, mcc, mnc):
        """
        Ensure that no two mcc, mnc pairs concat to the same MCCMNC.

        @param mcc: The mcc to check.
        @param mnc: The mnc to check.

        """
        mccmnc = mcc + mnc
        if mccmnc in self._mcc_mnc_by_mccmnc:
            old_mcc, old_mnc = self._mcc_mnc_by_mccmnc(mccmnc)
            if old_mcc != mcc or old_mnc != mnc:
                self._LogAndRaise(u'Ambiguous MCCMNC pairs detected: '
                                  u'(%s, %s) vs. (%s, %s)',
                                  old_mcc, old_mnc, mcc, mnc)

        self._mcc_mnc_by_mccmnc[u'mccmnc'] = (mcc, mnc)


    def _GroupGSMNodesByMCCMNC(self):
        """ Map all GSM nodes with same MCCMNC together. """
        for gsm_node in self._root.findall(u'.//gsm'):
            network_id_nodes = gsm_node.findall(u'network-id')
            if not network_id_nodes:
                logging.warning('Found a GSM node with no MCCMNC. ')
                self._gsm_nodes_no_mccmnc.add(gsm_node)
                continue

            for network_id_node in gsm_node.findall(u'network-id'):
                mcc = network_id_node.get(u'mcc')
                mnc = network_id_node.get(u'mnc')
                self._CheckAmbiguousMCCMNC(mcc, mnc)
                mccmnc = mcc + mnc
                if mccmnc in self._gsm_nodes_by_mccmnc:
                    self._gsm_nodes_by_mccmnc[mccmnc].append(gsm_node)
                else:
                    self._gsm_nodes_by_mccmnc[mccmnc] = [gsm_node]


    def _FindPrimaryNodes(self):
        """
        Finds nodes that correspond to MNOs as opposed to MVNOs.

        All CDMA nodes are primary, all GSM nodes that have a unique MCCMNC are
        primary, GSM nodes with non-unique MCCMNC that explicitly claim to be
        primary are primary.

        """
        unique_mvnos = set()
        self._mvnos = {}

        # All cdma nodes are primary.
        self._primary_cdma_nodes = set(self._root.findall(u'.//cdma'))

        self._protobuf_cdma_mnos = len(self._primary_cdma_nodes)


        # Start by marking all nodes with no MCCMNC primary.
        self._primary_gsm_nodes = self._gsm_nodes_no_mccmnc
        for mccmnc, nodes in self._gsm_nodes_by_mccmnc.iteritems():
            mvnos = set()
            if len(nodes) == 1:
                self._primary_gsm_nodes.add(nodes[0])
                continue

            # Exactly one node in the list should claim to be primary.
            primary = None
            for node in nodes:
                provider_node = node.find(u'provider')
                if (provider_node.get(u'primary') and
                    provider_node.get(u'primary') == u'true'):
                    if primary is not None:
                        self._LogAndRaise(
                                u'Found two primary gsm nodes with MCCMNC['
                                u'%s]: \n%s\n%s',
                                mccmnc, self._PPrintXML(primary),
                                self._PPrintXML(node))

                    primary = node
                    self._primary_gsm_nodes.add(node)
                else:
                    mvnos.add(node)
            if primary is None:
                logging.warning('Failed to find primary node with '
                                'MCCMNC[%s]. Will make all of them '
                                'distinct MNOs', mccmnc)
                logging.info('Nodes found:')
                for node in nodes:
                    self._PPrintLogXML(logging.info, node)
                self._primary_gsm_nodes = (self._primary_gsm_nodes | set(nodes))
                continue

            # This primary may already have MVNOs due to another MCCMNC.
            existing_mvnos = self._mvnos.get(primary, set())
            self._mvnos[primary] = existing_mvnos | mvnos
            # Only add to the MVNO count the *new* MVNOs added.
            self._protobuf_gsm_mvnos += (len(self._mvnos[primary]) -
                                         len(existing_mvnos))
            unique_mvnos = unique_mvnos | mvnos

        self._primary_nodes = (self._primary_cdma_nodes |
                               self._primary_gsm_nodes)
        self._protobuf_gsm_mnos = len(self._primary_gsm_nodes)
        self._protobuf_gsm_unique_mvnos = len(unique_mvnos)
        self._protobuf_gsm_mvnos_mnos = len(
                self._primary_gsm_nodes & unique_mvnos)


    def _SortOperators(self, node_list):
        """ Sort operators by country and name """
        # First sort by name.
        node_list.sort(cmp=lambda x, y:
                          cmp(sorted([z.text for z in x.findall(u'name')]),
                              sorted([z.text for z in y.findall(u'name')])))
        # Now sort by country. Since list sort is stable, nodes with the same
        # country remain sorted by name.
        node_list.sort(cmp=lambda x, y: cmp(x.find(u'country').get(u'code'),
                                            y.find(u'country').get(u'code')))


    def _SpewProtobuf(self):
        """ Entry function for dumping to prototext format. """
        _, fname = os.path.split(__file__)
        self._SpewComment("!!! DO NOT EDIT THIS FILE BY HAND !!!");
        self._SpewComment("This file is generated by the script %s" % fname)
        self._SpewComment("This file was generated from serviceproviders.xml, "
                          "a public domain database of cellular network "
                          "operators around the globe.")

        primaries = list(self._primary_nodes)
        self._SortOperators(primaries)
        for node in primaries:
            self._protobuf_mnos_dumped += 1
            self._SpewMessageBegin(u'mno')
            self._SpewData(node)
            if node in self._mvnos:
                mvnos = list(self._mvnos[node])
                self._SortOperators(mvnos)
                for mvno_node in mvnos:
                    self._protobuf_mvnos_dumped += 1
                    self._SpewMessageBegin(u'mvno')
                    self._SpewNameFilter(mvno_node)
                    self._SpewData(mvno_node)
                    self._SpewMessageEnd(u'mvno')
            self._SpewMessageEnd(u'mno')
            self._SpewLine()


    def _SpewNameFilter(self, node):
        name_list = []
        for name_node in node.findall(u'name'):
            if name_node.text:
                name_list.append(name_node.text)
        if not name_list:
            self._LogAndRaise(
                    u'Did not find any name for MVNO. Can not create filter.\n'
                    u'%s', self._PPrintXML(node))

        name = u'|'.join(name_list)
        self._SpewMessageBegin(u'mvno_filter')
        self._SpewEnum(u'type', u'OPERATOR_NAME')
        self._SpewString(u'regex', name)
        self._SpewMessageEnd(u'mvno_filter')


    def _SpewData(self, node):
        self._SpewMessageBegin(u'data')

        self._SpewString(u'uuid', str(uuid.uuid4()))
        country_node = node.find(u'country')
        self._SpewString(u'country', country_node.get(u'code'))

        provider_node = node.find(u'provider')
        provider_type = provider_node.get(u'type')
        self._SpewEnum(u'provider_type', provider_type.upper())
        roaming_required = provider_node.get(u'roaming-required')
        if roaming_required is not None:
            self._SpewBool(u'requires_roaming', roaming_required)
        for name_node in sorted(node.findall(u'name')):
            self._SpewLocalizedNameNode(name_node)

        # GSM specific fields.
        for network_id_node in sorted(node.findall(u'network-id')):
            self._SpewString(u'mccmnc',
                             network_id_node.get(u'mcc') +
                             network_id_node.get(u'mnc'))

        for apn_node in sorted(node.findall(u'apn')):
            self._SpewMobileAPNNode(apn_node)

        # CDMA specific fields.
        for sid_node in sorted(node.findall(u'sid')):
            self._SpewString(u'sid', sid_node.get(u'value'))

        # CDMA networks have some extra username/password/dns information that
        # corresponds very well with the APN concept of 3GPP, so we map it to an
        # MobileAPN instead of storing it specially.
        if (node.find(u'username') is not None or
            node.find(u'password') is not None or
            node.find(u'dns') is not None):
            self._SpewMobileAPNNode(node)

        self._SpewMessageEnd(u'Data')


    def _SpewMobileAPNNode(self, apn_node):
        self._SpewMessageBegin(u'mobile_apn')
        apn = apn_node.get(u'value')
        # This may be None when converting a <cdma> node to MobileAPN node.
        if apn is None:
            apn=''
        self._SpewString(u'apn', apn)
        for plan_node in sorted(apn_node.findall(u'plan')):
            self._SpewEnum(u'plan', plan_node.get(u'type').upper())
        for name_node in sorted(apn_node.findall(u'name')):
            self._SpewLocalizedNameNode(name_node)
        for gateway_node in apn_node.findall(u'gateway'):
            self._SpewString(u'gateway', gateway_node.text)
        for username_node in apn_node.findall(u'username'):
            self._SpewString(u'username', username_node.text)
        for password_node in apn_node.findall(u'password'):
            self._SpewString(u'password', password_node.text)
        for dns_node in sorted(apn_node.findall(u'dns')):
            self._SpewString(u'dns', dns_node.text)
        self._SpewMessageEnd(u'mobile_apn')


    def _SpewLocalizedNameNode(self, name_node):
        self._SpewMessageBegin(u'localized_name')
        self._SpewString(u'name', name_node.text)
        lang = name_node.get(u'xml:lang')
        if lang is not None:
            self._SpewString(u'language', lang)
        self._SpewMessageEnd(u'localized_name')


    def _SpewMessageBegin(self, message_name):
        self._SpewLine(message_name, u'{')
        self._indent += 1


    def _SpewMessageEnd(self, _):
        self._indent -= 1
        self._SpewLine(u'}')


    def _SpewString(self, key, value):
        # Treat None |value| as empty string.
        if value is None:
            value = u''
        self._SpewLine(key, u':', u'"' + value + u'"')


    def _SpewBool(self, key, value):
        self._SpewLine(key, u':', value)


    def _SpewEnum(self, key, value):
        self._SpewLine(key, u':', value)


    def _SpewComment(self, comment):
        line_length = 78 - (2 * self._indent)
        comment_lines = textwrap.wrap(comment, line_length)
        for line in comment_lines:
            self._SpewLine(u'# ' + line)


    def _SpewLine(self, *args):
        indent = (2 * self._indent) * u' '
        line = indent + u' '.join(args) + u'\n'
        self._out_file.write(line.encode(FILE_ENCODING))


    def _PPrintXML(self, node):
        """ Returns a pretty-printed |unicode| string for the xml |node|. """
        rough_string_str = ElementTree.tostring(node, encoding=FILE_ENCODING)
        reparsed = minidom.parseString(rough_string_str)
        xml_data_str = reparsed.toprettyxml(indent=u'  ',
                                            encoding=FILE_ENCODING)
        xml_data = unicode(xml_data_str, FILE_ENCODING)
        lines = xml_data.split(u'\n')
        lines = [line.strip(u'\n') for line in lines]
        lines = [line for line in lines if not line.strip() == u'']
        lines = [line.strip(u'\n') for line in lines if line.strip()]
        retval = u'\n'.join(lines)
        return retval


    def _PPrintLogXML(self, logger, node):
        """ Logs a given xml |node| to |logger| encoded in 'ascii' format. """
        to_print = self._PPrintXML(node)
        # Marshall, as best as we can to ASCII.
        to_print_str_ascii = to_print.encode('ascii', errors='replace')
        lines_str_ascii = to_print_str_ascii.split('\n')
        logger('NODE:')
        for line_str_ascii in lines_str_ascii:
            logger(line_str_ascii)


    def _LogAndRaise(self, fmt, *args):
        """
        Logs the error encoded in 'ascii' format and raises an error.

        @param fmt: The base formatted string for the error.
        @param *args: Arguments to format the string |fmt|.
        @raises ConverterError

        """
        error_string = fmt.format(*args)
        # Marshall, as best as we can to ASCII.
        error_string_str_ascii = error_string.encode('ascii', errors='replace')
        logging.error(error_string_str_ascii)
        raise ConverterError(error_string_str_ascii)


def main(prog_name, args):
    """
    Entry function to this script.

    @param prog_name: Name of the program to display.
    @param args: Command line arguments.

    """
    logging.basicConfig(level=logging.DEBUG)

    if not (1 <= len(args) <= 2):
        print("Usage: %s <in_file> [<out_file>]" % prog_name)
        sys.exit(1)

    in_file_path = args[0]
    out_file_path = args[1] if len(args) == 2 else None

    converter = ServiceProvidersConverter(in_file_path, out_file_path)
    converter.Convert()


if __name__ == '__main__':
    main(sys.argv[0], sys.argv[1:])
