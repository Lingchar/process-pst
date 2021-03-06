// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// process-pst: Convert PST files to RCF822 *.eml files and load files
// Copyright (c) 2010 Aranetic LLC
// Look for the latest version at http://github.com/aranetic/process-pst
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Affero General Public License (the "License")
// as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but it
// is provided on an "AS-IS" basis and WITHOUT ANY WARRANTY; without even
// the implied warranties of MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE, OR NONINFRINGEMENT.  See the GNU Affero General Public License
// for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include <sstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <pstsdk/pst.h>

using namespace std;
using namespace std::placeholders;
using namespace pstsdk;
using namespace boost::posix_time;

wstring string_to_wstring(const string &str) {
    // TODO: We need to call mbstowcs here.
    return wstring(str.begin(), str.end());
}

struct prop_id_name_info {
    prop_id id;
    const wchar_t *name;
};

// These values can be found in [MS-OXPROPS].pdf, along with some
// documentation about each field.
prop_id_name_info prop_name_map[] = {
    { 0x0002, L"PidTagAlternateRecipientAllowed" },
    { 0x0017, L"PidTagImportance" },
    { 0x001a, L"PidTagMessageClass" },
    { 0x0023, L"PidTagOriginatorDeliveryReportRequested" },
    { 0x0026, L"PidTagPriority" },
    { 0x0029, L"PidTagReadReceiptRequested" },
    { 0x002b, L"PidTagRecipientReassignmentProhibited" },
    { 0x002e, L"PidTagOriginalSensitivity" },
    { 0x0036, L"PidTagSensitivity" },
    { 0x0037, L"PidTagSubject" },
    { 0x0039, L"PidTagClientSubmitTime" },
    { 0x003b, L"PidTagSentRepresentingSearchKey" },
    { 0x003f, L"PidTagReceivedByEntryId" },
    { 0x0040, L"PidTagReceivedByName" },
    { 0x0041, L"PidTagSentRepresentingEntryId" },
    { 0x0042, L"PidTagSentRepresentingName" },
    { 0x0043, L"PidTagReceivedRepresentingEntryId" },
    { 0x0044, L"PidTagReceivedRepresentingName" },
    { 0x0047, L"PidTagMessageSubmissionId" },
    { 0x0051, L"PidTagReceivedBySearchKey" },
    { 0x0052, L"PidTagReceivedRepresentingSearchKey" },
    { 0x0057, L"PidTagMessageToMe" },
    { 0x0058, L"PidTagMessageCcMe" },
    { 0x0060, L"PidTagStartDate" },
    { 0x0061, L"PidTagEndDate" },
    { 0x0062, L"PidTagOwnerAppointmentId" },
    { 0x0063, L"PidTagResponseRequested" },
    { 0x0064, L"PidTagSentRepresentingAddressType" },
    { 0x0065, L"PidTagSentRepresentingEmailAddress" },
    { 0x0070, L"PidTagConversationTopic" },
    { 0x0071, L"PidTagConversationIndex" },
    { 0x0075, L"PidTagReceivedByAddressType" },
    { 0x0076, L"PidTagReceivedByEmailAddress" },
    { 0x0077, L"PidTagReceivedRepresentingAddressType" },
    { 0x0078, L"PidTagReceivedRepresentingEmailAddress" },
    { 0x007d, L"PidTagTransportMessageHeaders" },
    { 0x007f, L"PidTagTnefCorrelationKey" },
    { 0x0c15, L"PidTagRecipientType" },
    { 0x0c17, L"PidTagReplyRequested" },
    { 0x0c19, L"PidTagSenderEntryId" },
    { 0x0c1a, L"PidTagSenderName" },
    { 0x0c1d, L"PidTagSenderSearchKey" },
    { 0x0c1e, L"PidTagSenderAddressType" },
    { 0x0c1f, L"PidTagSenderEmailAddress" },
    { 0x0e01, L"PidTagDeleteAfterSubmit" },
    { 0x0e03, L"PidTagDisplayCc" },
    { 0x0e04, L"PidTagDisplayTo" },
    { 0x0e06, L"PidTagMessageDeliveryTime" },
    { 0x0e07, L"PidTagMessageFlags" },
    { 0x0e08, L"PidTagMessageSize" },
    { 0x0e0f, L"PidTagResponsibility" },
    { 0x0e1f, L"PidTagRtfInSync" },
    { 0x0e23, L"PidTagInternetArticleNumber" },
    { 0x0e27, L"PidTagSecurityDescriptor" },
    { 0x0e20, L"PidTagAttachSize" },
    { 0x0e2b, L"PidTagToDoItemFlags" },
    //{ 0x0e2f, L"" },
    { 0x0e34, L"PidTagReplVersionHistory" },
    { 0x0e38, L"PidTagReplFlags" },
    { 0x0e79, L"PidTagTrustSender" },
    { 0x0FF9, L"PidTagRecordKey" },
    { 0x0ffe, L"PidTagObjectType" },
    { 0x0fff, L"PidTagEntryId" },
    { 0x1000, L"PidTagBody" },
    { 0x1009, L"PidTagRtfCompressed" },
    { 0x1013, L"PidTagBodyHtml" },
    { 0x1035, L"PidTagInternetMessageId" },
    { 0x1080, L"PidTagIconIndex" },
    { 0x1090, L"PidTagFlagStatus" },
    // Prop 0x1095 allegedly "specifies the flag color of the Message object."
    { 0x1095, L"PidTagFollowupIcon" },
    { 0x10f4, L"PidTagAttributeHidden" },
    { 0x10f5, L"PidTagAttributeSystem" },
    { 0x10f6, L"PidTagAttributeReadOnly" },
    { 0x3001, L"PidTagDisplayName" },
    { 0x3002, L"PidTagAddressType" },
    { 0x3003, L"PidTagEmailAddress" },
    { 0x3007, L"PidTagCreationTime" },
    { 0x3008, L"PidTagLastModificationTime" },
    { 0x300b, L"PidTagSearchKey" },
    { 0x3010, L"PidTagTargetEntryId" },
    //{ 0x3014, L"" },
    //{ 0x3015, L"" },
    { 0x3016, L"PidTagConversationIndexTracking" },
    //{ 0x3416, L"" },
    //{ 0x35df, L"" },
    //{ 0x35e0, L"" },
    //{ 0x35e3, L"" },
    //{ 0x35e7, L"" },
    // If 0x3701 has type 0x000d, then it's PidTagAttachDataObject instead.
    { 0x3701, L"PidTagAttachDataBinary" },
    { 0x3702, L"PidTagAttachEncoding" },
    { 0x3704, L"PidTagAttachFilename" },
    { 0x3705, L"PidTagAttachMethod" },
    { 0x3707, L"PidTagAttachLongFilename" },
    { 0x370a, L"PidTagAttachTag" },
    { 0x370b, L"PidTagRenderingPosition" },
    { 0x370e, L"PidTagAttachMimeTag" },
    { 0x370f, L"PidTagAttachAdditionalInformation" },
    //{ 0x3710, L"" },
    { 0x3900, L"PidTagDisplayType" },
    { 0x3905, L"PidTagDisplayTypeEx" },
    { 0x39fe, L"PidTagPrimarySmtpAddress" },
    { 0x3a00, L"PidTagAccount" },
    { 0x3a20, L"PidTagTransmittableDisplayName" },
    { 0x3a40, L"PidTagSendRichInfo" },
    //{ 0x3d01, L"" },
    { 0x3fde, L"PidTagInternetCodepage" },
    { 0x3fea, L"PidTagHasDeferredActionMessages" },
    { 0x3ff1, L"PidTagMessageLocaleId" },
    { 0x3ff8, L"PidTagCreatorName" },
    { 0x3ff9, L"PidTagCreatorEntryId" },
    { 0x3ffa, L"PidTagLastModifierName" },
    { 0x3ffb, L"PidTagLastModifierEntryId" },
    { 0x3ffd, L"PidTagMessageCodepage" },
    { 0x4019, L"PidTagSenderFlags" },
    { 0x401a, L"PidTagSentRepresentingFlags" },
    { 0x401b, L"PidTagReceivedByFlags" },
    { 0x401c, L"PidTagReceivedRepresentingFlags" },
    //{ 0x405d, L"" },
    { 0x5902, L"PidTagInternetMailOverrideFormat" },
    { 0x5909, L"PidTagMessageEditorFormat" },
    { 0x5d01, L"PidTagSenderSmtpAddress" },
    //{ 0x5d02, L"" },
    { 0x5fde, L"PidTagRecipientResourceState" },
    { 0x5fdf, L"PidTagRecipientOrder" },
    { 0x5fe5, L"PidTagSessionInitiationProtocolUri" },
    //{ 0x5feb, L"" },
    //{ 0x5fef, L"" },
    //{ 0x5ff2, L"" },
    //{ 0x5ff5, L"" },
    { 0x5ff6, L"PidTagRecipientDisplayName" },
    { 0x5ff7, L"PidTagRecipientEntryId" },
    { 0x5ffd, L"PidTagRecipientFlags" },
    { 0x5fff, L"PidTagRecipientTrackStatus" },
    //{ 0x6540, L"" },
    { 0x65e2, L"PidTagChangeKey" },
    { 0x65e3, L"PidTagPredecessorChangeList" },
    //{ 0x6610, L"" },
    //{ 0x6619, L"" },  // Not PidTagUserEntryId, I don't think.
    { 0x6633, L"PidTagPstLrNoRestrictions" },
    { 0x66fa, L"PidTagLatestPstEnsure" },
    //{ 0x67f2, L"" },
    //{ 0x67f3, L"" },
    { 0x67ff, L"PidTagPstPassword" },
    //{ 0x6909, L"" },

    { 0, NULL }
};

/* ps_common: None of these are currently interesting.
prop_id_name_info common_header_map[] = {
    { 0x8501, L"PidLidReminderDelta" },
    { 0x8502, L"PidLidReminderTime" },
    { 0x8503, L"PidLidReminderSet" },
    { 0x8506, L"PidLidPrivate" },
    { 0x850e, L"PidLidAgingDontAgeMe" },
    { 0x8510, L"PidLidSideEffects" },
    { 0x8514, L"PidLidSmartNoAttach" },
    { 0x8516, L"PidLidCommonStart" },
    { 0x8517, L"PidLidCommonEnd" },
    { 0x8518, L"PidLidTaskMode" },
    { 0x8530, L"PidLidFlagRequest" },
    { 0x8552, L"PidLidCurrentVersion" },
    { 0x8554, L"PidLidCurrentVersionName" },
    { 0x8560, L"PidLidReminderSignalTime" },
    { 0x8580, L"PidLidInternetAccountName" },
    { 0x8581, L"PidLidInternetAccountStamp" },
    { 0x8582, L"PidLidUseTnef" },
    //{ 0x8596, L"" },
    { 0x85a0, L"PidLidToDoOrdinalDate" },
    { 0x85a1, L"PidLidToDoSubOrdinal" },
    { 0x85a4, L"PidLidToDoTitle" },
    { 0, NULL }
};
*/

const guid ps_common = { 0x62008, 0, 0, { 0xc0, 0, 0, 0, 0, 0, 0, 0x46 } };
const guid ps_internet_headers = { 0x20386, 0, 0, { 0xc0, 0, 0, 0, 0, 0, 0, 0x46 } };
const guid ps_task = { 0x62003, 0, 0, { 0xc0, 0, 0, 0, 0, 0, 0, 0x46 } };
const guid ps_messaging = { 0x41f28f13, static_cast<uint16_t>(0x83f4), 0x4114, { 0xa5, 0x84, 0xee, 0xdb, 0x5a, 0x6b, 0x0b, 0xff } };
const guid ps_appointment = { 0x62002, 0, 0, { 0xc0, 0, 0, 0, 0, 0, 0, 0x46 } };
const guid ps_meeting = { 0x6ed8da90, 0x450b, 0x101b, { 0x98, 0xda, 0, 0xaa, 0, 0x3f, 0x13, 0x05 } };

bool operator==(const guid g1, const guid g2) {
    return (g1.data1 == g2.data1 &&
            g1.data2 == g2.data2 &&
            g1.data3 == g2.data3 &&
            memcmp(g1.data4, g2.data4, 8) == 0);
}

wstring guid_name(guid g) {
    if (g == ps_none) {
        return L"ps_none";
    } else if (g == ps_mapi) {
        return L"ps_mapi";
    } else if (g == ps_public_strings) {
        return L"ps_public_strings";
    } else if (g == ps_common) {
        return L"ps_common";
    } else if (g == ps_internet_headers) {
        return L"ps_internet_headers";
    } else if (g == ps_task) {
        return L"ps_task";
    } else if (g == ps_messaging) {
        return L"ps_messaging";
    } else if (g == ps_appointment) {
        return L"ps_appointment";
    } else if (g == ps_meeting) {
        return L"ps_meeting";
    } else {
        wostringstream out;
        out << hex
            << setw(8) << setfill(L'0') << g.data1 << L"-"
            << setw(4) << setfill(L'0') << g.data2 << L"-"
            << setw(4) << setfill(L'0') << g.data3 << L"-";
        for (size_t i = 0; i < 8; i++)
            out << setw(2) << setfill(L'0') << g.data4[i];
        return out.str();
    }
}

shared_db_ptr g_db;

wstring property_name(prop_id id) {
    if (id >= 0x8000) {
        name_id_map prop_names(g_db);
        if (prop_names.prop_id_exists(id)) {
            named_prop prop(prop_names.lookup(id));
            wostringstream out;
            out << guid_name(prop.get_guid()) << L" ";
            if (prop.is_string())
                out << prop.get_name();
            else
                out << L"0x" << hex << setw(4) << setfill(L'0')
                    << prop.get_id();
            return out.str();
        }
    } else {
        // Look this up in our map of known names.
        for (prop_id_name_info *i = prop_name_map; i->name != NULL; i++)
            if (i->id == id)
                return i->name;
    }
        
    ostringstream out;
    out << "0x" << hex << setw(4) << setfill('0') << id;
    return string_to_wstring(out.str());
}

template <typename R, typename T, typename D>
R prop_or(const T &obj, R (T::*pmf)() const, D default_value) {
    try {
        return (obj.*pmf)();
    } catch (key_not_found<prop_id> &) {
        return default_value;
    }
}

void process_property(const const_property_object *props, size_t level,
                      prop_id id) {
    for (size_t i = 0; i < level; ++i)
        wcout << L"  ";
    wcout << property_name(id) << ": ";
    prop_type type(props->get_prop_type(id));
    switch (type) {
        case prop_type_null:
            wcout << "null";
            break;

        case prop_type_short:
            wcout << props->read_prop<boost::int16_t>(id);
            break;

        case prop_type_long:
            wcout << props->read_prop<boost::int32_t>(id);
            break;

        case prop_type_float:
            wcout << props->read_prop<float>(id);
            break;

        case prop_type_double:
            wcout << props->read_prop<double>(id);
            break;

        case prop_type_boolean:
            wcout << (props->read_prop<bool>(id) ? L"true" : L"false");
            break;

        case prop_type_longlong:
            wcout << props->read_prop<boost::int64_t>(id);
            break;

        case prop_type_string:
        case prop_type_wstring:
            wcout << props->read_prop<wstring>(id);
            break;

        case prop_type_apptime:
        case prop_type_systime:
            wcout << from_time_t(props->read_time_t_prop(id));
            break;

        case prop_type_binary: {
            vector<byte> data(props->read_prop<vector<byte> >(id));
            wcout << hex;
            vector<byte>::iterator i(data.begin());
            for (; i != data.end() && i - data.begin() < 25; ++i)
                wcout << setw(2) << setfill(L'0') << *i;
            if (i != data.end())
                wcout << L"...";
            wcout << dec;
            break;
        }

        default:
            wcout << L"(Unsupported value of type " << type << L")";
            break;
    }
    wcout << endl;
}

void process_properties(const const_property_object *props, size_t level) {
    std::vector<prop_id> ids(props->get_prop_list());
    for_each(ids.begin(), ids.end(), bind(process_property, props, level, _1));
}

void process_node_properties(const property_bag *props, size_t level) {
    for (size_t i = 0; i < level; ++i)
        wcout << L"  ";
    wcout << L"PidTagEntryId(*): " << hex;
    vector<byte> entry_id(props->get_entry_id());
    vector<byte>::iterator i(entry_id.begin());
    for (; i != entry_id.end(); ++i)
        wcout << setw(2) << setfill(L'0') << *i;
    wcout << dec << endl;
    process_properties(props, level);
}

void process_recipient(const recipient &r) {
    wcout << "  ";
    switch (prop_or(r, &recipient::get_type, recipient_type(0))) {
        case mapi_to: wcout << "To"; break;
        case mapi_cc: wcout << "CC"; break;
        case mapi_bcc: wcout << "BCC"; break;
        default: wcout << "(Unknown type)"; break;
    }
    wcout << ": " << prop_or(r, &recipient::get_name, L"(anonymous)") << " <"
          << prop_or(r, &recipient::get_email_address, L"(no email)")
          << ">" << endl;
    process_properties(&r.get_property_row(), 2);
}

void process_attachment(const attachment &a) {
    wcout << "  Attachment: "
          << prop_or(a, &attachment::get_filename, L"(no filename)") << " ("
          << prop_or(a, &attachment::size, 31337) << " bytes)"
          << (a.is_message() ? " SUBMESSAGE" : "")
          << endl;
    process_node_properties(&a.get_property_bag(), 2);
}

void process_message(const message &m) {
    wcout << prop_or(m, &message::get_subject, L"(null)") << endl;

    if (m.get_recipient_count() > 0)
        for_each(m.recipient_begin(), m.recipient_end(), process_recipient);
    if (m.get_attachment_count() > 0)
        for_each(m.attachment_begin(), m.attachment_end(), process_attachment);

    process_node_properties(&m.get_property_bag(), 1);
}

void process_folder(const folder &f) {
    // This slick little approach is adapted from the pstsdk sample code.
    for_each(f.message_begin(), f.message_end(), process_message);
    for_each(f.sub_folder_begin(), f.sub_folder_end(), process_folder);
}

int main(int argc, char **argv) {
    // Parse our command-line arguments.
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " input.pst..." << endl;
        exit(1);
    }

    // Open our PSTs.
    for (int i = 1; i < argc; ++i) {
        string pst_path(argv[i]);
        pst pst_db(string_to_wstring(pst_path));
        g_db = pst_db.get_db();
        process_properties(&pst_db.get_property_bag(), 0);
        process_folder(pst_db.open_root_folder());
        g_db.reset();
    }
}
