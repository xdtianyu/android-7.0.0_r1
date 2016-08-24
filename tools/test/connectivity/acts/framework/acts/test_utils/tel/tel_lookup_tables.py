#!/usr/bin/env python3.4
#
#   Copyright 2016 - Google
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
from acts.utils import NexusModelNames
from acts.test_utils.tel import tel_defines


def rat_family_from_rat(rat_type):
    return _TelTables.technology_tbl[rat_type]['rat_family']


def rat_generation_from_rat(rat_type):
    return _TelTables.technology_tbl[rat_type]['generation']


def network_preference_for_generaton(generation, operator):
    return _TelTables.operator_network_tbl[operator][generation][
        'network_preference']


def rat_families_for_network_preference(network_preference):
    return _TelTables.network_preference_tbl[network_preference][
        'rat_family_list']


def rat_family_for_generation(generation, operator):
    return _TelTables.operator_network_tbl[operator][generation]['rat_family']


def operator_name_from_plmn_id(plmn_id):
    return _TelTables.operator_id_to_name[plmn_id]


def is_valid_rat(rat_type):
    return True if rat_type in _TelTables.technology_tbl else False


def is_valid_generation(gen):
    return True if gen in _TelTables.technology_gen_tbl else False


def is_rat_svd_capable(rat):
    return _TelTables.technology_tbl[rat]["simultaneous_voice_data"]


def connection_type_from_type_string(input_string):
    if input_string in _ConnectionTables.connection_type_tbl:
        return _ConnectionTables.connection_type_tbl[input_string]
    return tel_defines.NETWORK_CONNECTION_TYPE_UNKNOWN


def is_user_plane_data_type(connection_type):
    if connection_type in _ConnectionTables.user_plane_data_type:
        return _ConnectionTables.user_plane_data_type[connection_type]
    return False


# For TMO, to check if voice mail count is correct after leaving a new voice message.
def check_tmo_voice_mail_count(voice_mail_count_before,
                               voice_mail_count_after):
    return (voice_mail_count_after == -1)


# For ATT, to check if voice mail count is correct after leaving a new voice message.
def check_att_voice_mail_count(voice_mail_count_before,
                               voice_mail_count_after):
    return (voice_mail_count_after == (voice_mail_count_before + 1))


# For SPT, to check if voice mail count is correct after leaving a new voice message.
def check_spt_voice_mail_count(voice_mail_count_before,
                               voice_mail_count_after):
    return (voice_mail_count_after == (voice_mail_count_before + 1))


# For TMO, get the voice mail number
def get_tmo_voice_mail_number():
    return "123"


# For ATT, get the voice mail number
def get_att_voice_mail_number():
    return None


# For SPT, get the voice mail number
def get_spt_voice_mail_number():
    return None


def get_voice_mail_number_function(operator):
    return _TelTables.voice_mail_number_get_function_tbl[operator]


def get_voice_mail_count_check_function(operator):
    return _TelTables.voice_mail_count_check_function_tbl[operator]


def get_allowable_network_preference(operator):
    return _TelTables.allowable_network_preference_tbl[operator]


class _ConnectionTables():
    connection_type_tbl = {
        'WIFI': tel_defines.NETWORK_CONNECTION_TYPE_WIFI,
        'WIFI_P2P': tel_defines.NETWORK_CONNECTION_TYPE_WIFI,
        'MOBILE': tel_defines.NETWORK_CONNECTION_TYPE_CELL,
        'MOBILE_DUN': tel_defines.NETWORK_CONNECTION_TYPE_CELL,
        'MOBILE_HIPRI': tel_defines.NETWORK_CONNECTION_TYPE_HIPRI,
        # TODO: b/26296489 add support for 'MOBILE_SUPL', 'MOBILE_HIPRI',
        # 'MOBILE_FOTA', 'MOBILE_IMS', 'MOBILE_CBS', 'MOBILE_IA',
        # 'MOBILE_EMERGENCY'
        'MOBILE_MMS': tel_defines.NETWORK_CONNECTION_TYPE_MMS
    }

    user_plane_data_type = {
        tel_defines.NETWORK_CONNECTION_TYPE_WIFI: True,
        tel_defines.NETWORK_CONNECTION_TYPE_CELL: False,
        tel_defines.NETWORK_CONNECTION_TYPE_MMS: False,
        tel_defines.NETWORK_CONNECTION_TYPE_UNKNOWN: False
    }


class _TelTables():
    # Operator id mapping to operator name
    # Reference: Pages 43-50 in
    # https://www.itu.int/dms_pub/itu-t/opb/sp/T-SP-E.212B-2013-PDF-E.pdf [2013]

    operator_id_to_name = {

        #VZW (Verizon Wireless)
        '310010': tel_defines.CARRIER_VZW,
        '310012': tel_defines.CARRIER_VZW,
        '310013': tel_defines.CARRIER_VZW,
        '310590': tel_defines.CARRIER_VZW,
        '310890': tel_defines.CARRIER_VZW,
        '310910': tel_defines.CARRIER_VZW,
        '310110': tel_defines.CARRIER_VZW,
        '311270': tel_defines.CARRIER_VZW,
        '311271': tel_defines.CARRIER_VZW,
        '311272': tel_defines.CARRIER_VZW,
        '311273': tel_defines.CARRIER_VZW,
        '311274': tel_defines.CARRIER_VZW,
        '311275': tel_defines.CARRIER_VZW,
        '311276': tel_defines.CARRIER_VZW,
        '311277': tel_defines.CARRIER_VZW,
        '311278': tel_defines.CARRIER_VZW,
        '311279': tel_defines.CARRIER_VZW,
        '311280': tel_defines.CARRIER_VZW,
        '311281': tel_defines.CARRIER_VZW,
        '311282': tel_defines.CARRIER_VZW,
        '311283': tel_defines.CARRIER_VZW,
        '311284': tel_defines.CARRIER_VZW,
        '311285': tel_defines.CARRIER_VZW,
        '311286': tel_defines.CARRIER_VZW,
        '311287': tel_defines.CARRIER_VZW,
        '311288': tel_defines.CARRIER_VZW,
        '311289': tel_defines.CARRIER_VZW,
        '311390': tel_defines.CARRIER_VZW,
        '311480': tel_defines.CARRIER_VZW,
        '311481': tel_defines.CARRIER_VZW,
        '311482': tel_defines.CARRIER_VZW,
        '311483': tel_defines.CARRIER_VZW,
        '311484': tel_defines.CARRIER_VZW,
        '311485': tel_defines.CARRIER_VZW,
        '311486': tel_defines.CARRIER_VZW,
        '311487': tel_defines.CARRIER_VZW,
        '311488': tel_defines.CARRIER_VZW,
        '311489': tel_defines.CARRIER_VZW,

        #TMO (T-Mobile USA)
        '310160': tel_defines.CARRIER_TMO,
        '310200': tel_defines.CARRIER_TMO,
        '310210': tel_defines.CARRIER_TMO,
        '310220': tel_defines.CARRIER_TMO,
        '310230': tel_defines.CARRIER_TMO,
        '310240': tel_defines.CARRIER_TMO,
        '310250': tel_defines.CARRIER_TMO,
        '310260': tel_defines.CARRIER_TMO,
        '310270': tel_defines.CARRIER_TMO,
        '310310': tel_defines.CARRIER_TMO,
        '310490': tel_defines.CARRIER_TMO,
        '310660': tel_defines.CARRIER_TMO,
        '310800': tel_defines.CARRIER_TMO,

        #ATT (AT&T and Cingular)
        '310070': tel_defines.CARRIER_ATT,
        '310560': tel_defines.CARRIER_ATT,
        '310670': tel_defines.CARRIER_ATT,
        '310680': tel_defines.CARRIER_ATT,
        '310150': tel_defines.CARRIER_ATT,  #Cingular
        '310170': tel_defines.CARRIER_ATT,  #Cingular
        '310410': tel_defines.CARRIER_ATT,  #Cingular
        '311180':
        tel_defines.CARRIER_ATT,  #Cingular Licensee Pacific Telesis Mobile Services, LLC

        #Sprint (and Sprint-Nextel)
        '310120': tel_defines.CARRIER_SPT,
        '311490': tel_defines.CARRIER_SPT,
        '311870': tel_defines.CARRIER_SPT,
        '311880': tel_defines.CARRIER_SPT,
        '312190': tel_defines.CARRIER_SPT,  #Sprint-Nextel Communications Inc
        '316010': tel_defines.CARRIER_SPT,  #Sprint-Nextel Communications Inc
        '23433': tel_defines.CARRIER_EEUK,  #Orange
        '23434': tel_defines.CARRIER_EEUK,  #Orange
        '23430': tel_defines.CARRIER_EEUK,  #T-Mobile UK
        '23431': tel_defines.CARRIER_EEUK,  #Virgin Mobile (MVNO)
        '23432': tel_defines.CARRIER_EEUK,  #Virgin Mobile (MVNO)
        '23415': tel_defines.CARRIER_VFUK
    }

    technology_gen_tbl = [tel_defines.GEN_2G, tel_defines.GEN_3G,
                          tel_defines.GEN_4G]

    technology_tbl = {
        tel_defines.RAT_1XRTT: {
            'is_voice_rat': True,
            'is_data_rat': False,
            'generation': tel_defines.GEN_3G,
            'simultaneous_voice_data': False,
            'rat_family': tel_defines.RAT_FAMILY_CDMA2000
        },
        tel_defines.RAT_EDGE: {
            'is_voice_rat': False,
            'is_data_rat': True,
            'generation': tel_defines.GEN_2G,
            'simultaneous_voice_data': False,
            'rat_family': tel_defines.RAT_FAMILY_GSM
        },
        tel_defines.RAT_GPRS: {
            'is_voice_rat': False,
            'is_data_rat': True,
            'generation': tel_defines.GEN_2G,
            'simultaneous_voice_data': False,
            'rat_family': tel_defines.RAT_FAMILY_GSM
        },
        tel_defines.RAT_GSM: {
            'is_voice_rat': True,
            'is_data_rat': False,
            'generation': tel_defines.GEN_2G,
            'simultaneous_voice_data': False,
            'rat_family': tel_defines.RAT_FAMILY_GSM
        },
        tel_defines.RAT_UMTS: {
            'is_voice_rat': True,
            'is_data_rat': True,
            'generation': tel_defines.GEN_3G,
            'simultaneous_voice_data': True,
            'rat_family': tel_defines.RAT_FAMILY_WCDMA
        },
        tel_defines.RAT_WCDMA: {
            'is_voice_rat': True,
            'is_data_rat': True,
            'generation': tel_defines.GEN_3G,
            'simultaneous_voice_data': True,
            'rat_family': tel_defines.RAT_FAMILY_WCDMA
        },
        tel_defines.RAT_HSDPA: {
            'is_voice_rat': False,
            'is_data_rat': True,
            'generation': tel_defines.GEN_3G,
            'simultaneous_voice_data': False,
            'rat_family': tel_defines.RAT_FAMILY_WCDMA
        },
        tel_defines.RAT_HSUPA: {
            'is_voice_rat': False,
            'is_data_rat': True,
            'generation': tel_defines.GEN_3G,
            'simultaneous_voice_data': False,
            'rat_family': tel_defines.RAT_FAMILY_WCDMA
        },
        tel_defines.RAT_CDMA: {
            'is_voice_rat': True,
            'is_data_rat': False,
            'generation': tel_defines.GEN_2G,
            'simultaneous_voice_data': False,
            'rat_family': tel_defines.RAT_FAMILY_CDMA
        },
        tel_defines.RAT_EVDO: {
            'is_voice_rat': False,
            'is_data_rat': True,
            'generation': tel_defines.GEN_3G,
            'simultaneous_voice_data': False,
            'rat_family': tel_defines.RAT_FAMILY_CDMA2000
        },
        tel_defines.RAT_EVDO_0: {
            'is_voice_rat': False,
            'is_data_rat': True,
            'generation': tel_defines.GEN_3G,
            'simultaneous_voice_data': False,
            'rat_family': tel_defines.RAT_FAMILY_CDMA2000
        },
        tel_defines.RAT_EVDO_A: {
            'is_voice_rat': False,
            'is_data_rat': True,
            'generation': tel_defines.GEN_3G,
            'simultaneous_voice_data': False,
            'rat_family': tel_defines.RAT_FAMILY_CDMA2000
        },
        tel_defines.RAT_EVDO_B: {
            'is_voice_rat': False,
            'is_data_rat': True,
            'generation': tel_defines.GEN_3G,
            'simultaneous_voice_data': False,
            'rat_family': tel_defines.RAT_FAMILY_CDMA2000
        },
        tel_defines.RAT_IDEN: {
            'is_voice_rat': False,
            'is_data_rat': True,
            'generation': tel_defines.GEN_2G,
            'simultaneous_voice_data': False,
            'rat_family': tel_defines.RAT_FAMILY_IDEN
        },
        tel_defines.RAT_LTE: {
            'is_voice_rat': True,
            'is_data_rat': True,
            'generation': tel_defines.GEN_4G,
            'simultaneous_voice_data': True,
            'rat_family': tel_defines.RAT_FAMILY_LTE
        },
        tel_defines.RAT_EHRPD: {
            'is_voice_rat': False,
            'is_data_rat': True,
            'generation': tel_defines.GEN_3G,
            'simultaneous_voice_data': False,
            'rat_family': tel_defines.RAT_FAMILY_CDMA2000
        },
        tel_defines.RAT_HSPA: {
            'is_voice_rat': False,
            'is_data_rat': True,
            'generation': tel_defines.GEN_3G,
            'simultaneous_voice_data': True,
            'rat_family': tel_defines.RAT_FAMILY_WCDMA
        },
        tel_defines.RAT_HSPAP: {
            'is_voice_rat': False,
            'is_data_rat': True,
            'generation': tel_defines.GEN_3G,
            'simultaneous_voice_data': True,
            'rat_family': tel_defines.RAT_FAMILY_WCDMA
        },
        tel_defines.RAT_IWLAN: {
            'is_voice_rat': True,
            'is_data_rat': True,
            'generation': tel_defines.GEN_4G,
            'simultaneous_voice_data': True,
            'rat_family': tel_defines.RAT_FAMILY_WLAN
        },
        tel_defines.RAT_TD_SCDMA: {
            'is_voice_rat': True,
            'is_data_rat': True,
            'generation': tel_defines.GEN_3G,
            'simultaneous_voice_data': True,
            'rat_family': tel_defines.RAT_FAMILY_TDSCDMA
        },
        tel_defines.RAT_UNKNOWN: {
            'is_voice_rat': False,
            'is_data_rat': False,
            'generation': tel_defines.GEN_UNKNOWN,
            'simultaneous_voice_data': False,
            'rat_family': tel_defines.RAT_FAMILY_UNKNOWN
        },
        tel_defines.RAT_GLOBAL: {
            'is_voice_rat': False,
            'is_data_rat': False,
            'generation': tel_defines.GEN_UNKNOWN,
            'simultaneous_voice_data': False,
            'rat_family': tel_defines.RAT_FAMILY_UNKNOWN
        }
    }

    network_preference_tbl = {
        tel_defines.NETWORK_MODE_LTE_GSM_WCDMA: {
            'rat_family_list': [tel_defines.RAT_FAMILY_LTE,
                                tel_defines.RAT_FAMILY_WCDMA,
                                tel_defines.RAT_FAMILY_GSM]
        },
        tel_defines.NETWORK_MODE_GSM_UMTS: {
            'rat_family_list': [tel_defines.RAT_FAMILY_WCDMA,
                                tel_defines.RAT_FAMILY_GSM]
        },
        tel_defines.NETWORK_MODE_GSM_ONLY: {
            'rat_family_list': [tel_defines.RAT_FAMILY_GSM]
        },
        tel_defines.NETWORK_MODE_LTE_CDMA_EVDO: {
            'rat_family_list': [tel_defines.RAT_FAMILY_LTE,
                                tel_defines.RAT_FAMILY_CDMA2000,
                                tel_defines.RAT_FAMILY_CDMA]
        },
        tel_defines.NETWORK_MODE_CDMA: {
            'rat_family_list': [tel_defines.RAT_FAMILY_CDMA2000,
                                tel_defines.RAT_FAMILY_CDMA]
        },
        tel_defines.NETWORK_MODE_CDMA_NO_EVDO: {
            'rat_family_list': [tel_defines.RAT_FAMILY_CDMA2000,
                                tel_defines.RAT_FAMILY_CDMA]
        },
        tel_defines.NETWORK_MODE_WCDMA_PREF: {
            'rat_family_list': [tel_defines.RAT_FAMILY_WCDMA,
                                tel_defines.RAT_FAMILY_GSM]
        },
        tel_defines.NETWORK_MODE_WCDMA_ONLY: {
            'rat_family_list': [tel_defines.RAT_FAMILY_WCDMA]
        },
        tel_defines.NETWORK_MODE_EVDO_NO_CDMA: {
            'rat_family_list': [tel_defines.RAT_FAMILY_CDMA2000]
        },
        tel_defines.NETWORK_MODE_GLOBAL: {
            'rat_family_list':
            [tel_defines.RAT_FAMILY_LTE, tel_defines.RAT_FAMILY_TDSCDMA,
             tel_defines.RAT_FAMILY_WCDMA, tel_defines.RAT_FAMILY_GSM,
             tel_defines.RAT_FAMILY_CDMA2000, tel_defines.RAT_FAMILY_CDMA]
        },
        tel_defines.NETWORK_MODE_LTE_CDMA_EVDO_GSM_WCDMA: {
            'rat_family_list':
            [tel_defines.RAT_FAMILY_LTE, tel_defines.RAT_FAMILY_WCDMA,
             tel_defines.RAT_FAMILY_GSM, tel_defines.RAT_FAMILY_CDMA2000,
             tel_defines.RAT_FAMILY_CDMA]
        },
        tel_defines.NETWORK_MODE_LTE_ONLY: {
            'rat_family_list': [tel_defines.RAT_FAMILY_LTE]
        },
        tel_defines.NETWORK_MODE_LTE_WCDMA: {
            'rat_family_list': [tel_defines.RAT_FAMILY_LTE,
                                tel_defines.RAT_FAMILY_WCDMA]
        },
        tel_defines.NETWORK_MODE_TDSCDMA_ONLY: {
            'rat_family_list': [tel_defines.RAT_FAMILY_TDSCDMA]
        },
        tel_defines.NETWORK_MODE_TDSCDMA_WCDMA: {
            'rat_family_list': [tel_defines.RAT_FAMILY_TDSCDMA,
                                tel_defines.RAT_FAMILY_WCDMA]
        },
        tel_defines.NETWORK_MODE_LTE_TDSCDMA: {
            'rat_family_list': [tel_defines.RAT_FAMILY_LTE,
                                tel_defines.RAT_FAMILY_TDSCDMA]
        },
        tel_defines.NETWORK_MODE_TDSCDMA_GSM: {
            'rat_family_list': [tel_defines.RAT_FAMILY_TDSCDMA,
                                tel_defines.RAT_FAMILY_GSM]
        },
        tel_defines.NETWORK_MODE_LTE_TDSCDMA_GSM: {
            'rat_family_list': [tel_defines.RAT_FAMILY_LTE,
                                tel_defines.RAT_FAMILY_TDSCDMA,
                                tel_defines.RAT_FAMILY_GSM]
        },
        tel_defines.NETWORK_MODE_TDSCDMA_GSM_WCDMA: {
            'rat_family_list': [tel_defines.RAT_FAMILY_WCDMA,
                                tel_defines.RAT_FAMILY_TDSCDMA,
                                tel_defines.RAT_FAMILY_GSM]
        },
        tel_defines.NETWORK_MODE_LTE_TDSCDMA_WCDMA: {
            'rat_family_list': [tel_defines.RAT_FAMILY_WCDMA,
                                tel_defines.RAT_FAMILY_TDSCDMA,
                                tel_defines.RAT_FAMILY_LTE]
        },
        tel_defines.NETWORK_MODE_LTE_TDSCDMA_GSM_WCDMA: {
            'rat_family_list':
            [tel_defines.RAT_FAMILY_WCDMA, tel_defines.RAT_FAMILY_TDSCDMA,
             tel_defines.RAT_FAMILY_LTE, tel_defines.RAT_FAMILY_GSM]
        },
        tel_defines.NETWORK_MODE_TDSCDMA_CDMA_EVDO_WCDMA: {
            'rat_family_list':
            [tel_defines.RAT_FAMILY_WCDMA, tel_defines.RAT_FAMILY_TDSCDMA,
             tel_defines.RAT_FAMILY_CDMA2000, tel_defines.RAT_FAMILY_CDMA]
        },
        tel_defines.NETWORK_MODE_LTE_TDSCDMA_CDMA_EVDO_GSM_WCDMA: {
            'rat_family_list':
            [tel_defines.RAT_FAMILY_WCDMA, tel_defines.RAT_FAMILY_TDSCDMA,
             tel_defines.RAT_FAMILY_LTE, tel_defines.RAT_FAMILY_GSM,
             tel_defines.RAT_FAMILY_CDMA2000, tel_defines.RAT_FAMILY_CDMA]
        }
    }
    default_umts_operator_network_tbl = {
        tel_defines.GEN_4G: {
            'rat_family': tel_defines.RAT_FAMILY_LTE,
            'network_preference': tel_defines.NETWORK_MODE_LTE_GSM_WCDMA
        },
        tel_defines.GEN_3G: {
            'rat_family': tel_defines.RAT_FAMILY_WCDMA,
            'network_preference': tel_defines.NETWORK_MODE_GSM_UMTS
        },
        tel_defines.GEN_2G: {
            'rat_family': tel_defines.RAT_FAMILY_GSM,
            'network_preference': tel_defines.NETWORK_MODE_GSM_ONLY
        }
    }
    default_cdma_operator_network_tbl = {
        tel_defines.GEN_4G: {
            'rat_family': tel_defines.RAT_FAMILY_LTE,
            'network_preference': tel_defines.NETWORK_MODE_LTE_CDMA_EVDO
        },
        tel_defines.GEN_3G: {
            'rat_family': tel_defines.RAT_FAMILY_CDMA2000,
            'network_preference': tel_defines.NETWORK_MODE_CDMA
        },
        tel_defines.GEN_2G: {
            'rat_family': tel_defines.RAT_FAMILY_CDMA2000,
            'network_preference': tel_defines.NETWORK_MODE_CDMA_NO_EVDO
        }
    }
    operator_network_tbl = {
        tel_defines.CARRIER_TMO: default_umts_operator_network_tbl,
        tel_defines.CARRIER_ATT: default_umts_operator_network_tbl,
        tel_defines.CARRIER_VZW: default_cdma_operator_network_tbl,
        tel_defines.CARRIER_SPT: default_cdma_operator_network_tbl,
        tel_defines.CARRIER_EEUK: default_umts_operator_network_tbl,
        tel_defines.CARRIER_VFUK: default_umts_operator_network_tbl
    }

    umts_allowable_network_preference_tbl = \
        [tel_defines.NETWORK_MODE_LTE_GSM_WCDMA,
         tel_defines.NETWORK_MODE_WCDMA_PREF,
         tel_defines.NETWORK_MODE_GSM_ONLY]

    cdma_allowable_network_preference_tbl = \
        [tel_defines.NETWORK_MODE_LTE_CDMA_EVDO,
         tel_defines.NETWORK_MODE_CDMA,
         tel_defines.NETWORK_MODE_CDMA_NO_EVDO,
         tel_defines.NETWORK_MODE_LTE_CDMA_EVDO_GSM_WCDMA]

    allowable_network_preference_tbl = {
        tel_defines.CARRIER_TMO: umts_allowable_network_preference_tbl,
        tel_defines.CARRIER_ATT: umts_allowable_network_preference_tbl,
        tel_defines.CARRIER_VZW: cdma_allowable_network_preference_tbl,
        tel_defines.CARRIER_SPT: cdma_allowable_network_preference_tbl,
        tel_defines.CARRIER_EEUK: umts_allowable_network_preference_tbl,
        tel_defines.CARRIER_VFUK: umts_allowable_network_preference_tbl
    }

    voice_mail_number_get_function_tbl = {
        tel_defines.CARRIER_TMO: get_tmo_voice_mail_number,
        tel_defines.CARRIER_ATT: get_att_voice_mail_number,
        tel_defines.CARRIER_SPT: get_spt_voice_mail_number
    }

    voice_mail_count_check_function_tbl = {
        tel_defines.CARRIER_TMO: check_tmo_voice_mail_count,
        tel_defines.CARRIER_ATT: check_att_voice_mail_count,
        tel_defines.CARRIER_SPT: check_spt_voice_mail_count
    }


device_capabilities = {
    NexusModelNames.ONE:
    [tel_defines.CAPABILITY_PHONE, tel_defines.CAPABILITY_MSIM],
    NexusModelNames.N5: [tel_defines.CAPABILITY_PHONE],
    NexusModelNames.N5v2:
    [tel_defines.CAPABILITY_PHONE, tel_defines.CAPABILITY_OMADM,
     tel_defines.CAPABILITY_VOLTE, tel_defines.CAPABILITY_WFC,
     tel_defines.CAPABILITY_VT],
    NexusModelNames.N6: [tel_defines.CAPABILITY_PHONE,
                         tel_defines.CAPABILITY_OMADM,
                         tel_defines.CAPABILITY_VOLTE,
                         tel_defines.CAPABILITY_WFC,
                         tel_defines.CAPABILITY_VT],
    NexusModelNames.N6v2: [tel_defines.CAPABILITY_PHONE,
                           tel_defines.CAPABILITY_OMADM,
                           tel_defines.CAPABILITY_VOLTE,
                           tel_defines.CAPABILITY_WFC,
                           tel_defines.CAPABILITY_VT]
}

operator_capabilities = {
    tel_defines.CARRIER_VZW: [tel_defines.CAPABILITY_PHONE,
                              tel_defines.CAPABILITY_OMADM,
                              tel_defines.CAPABILITY_VOLTE,
                              tel_defines.CAPABILITY_VT],
    tel_defines.CARRIER_ATT: [tel_defines.CAPABILITY_PHONE],
    tel_defines.CARRIER_TMO: [tel_defines.CAPABILITY_PHONE,
                              tel_defines.CAPABILITY_VOLTE,
                              tel_defines.CAPABILITY_WFC],
    tel_defines.CARRIER_SPT: [tel_defines.CAPABILITY_PHONE],
    tel_defines.CARRIER_EEUK: [tel_defines.CAPABILITY_PHONE],
    tel_defines.CARRIER_VFUK: [tel_defines.CAPABILITY_PHONE]
}
