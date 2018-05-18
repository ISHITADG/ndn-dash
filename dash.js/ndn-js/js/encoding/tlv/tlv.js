/**
 * Copyright (C) 2014-2016 Regents of the University of California.
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * A copy of the GNU Lesser General Public License is in the file COPYING.
 */

/**
 * The Tlv class has static type codes for the NDN-TLV wire format.
 * @constructor
 */
var Tlv = function Tlv()
{
};

exports.Tlv = Tlv;

Tlv.Interest =         5;
Tlv.Data =             6;
Tlv.Name =             7;
Tlv.NameComponent =    8;
Tlv.Selectors =        9;
Tlv.Nonce =            10;
// <Unassigned> =      11;
Tlv.InterestLifetime = 12;
Tlv.MinSuffixComponents = 13;
Tlv.MaxSuffixComponents = 14;
Tlv.PublisherPublicKeyLocator = 15;
Tlv.Exclude =          16;
Tlv.ChildSelector =    17;
Tlv.MustBeFresh =      18;
Tlv.Any =              19;
Tlv.MetaInfo =         20;
Tlv.Content =          21;
Tlv.SignatureInfo =    22;
Tlv.SignatureValue =   23;
Tlv.ContentType =      24;
Tlv.FreshnessPeriod =  25;
Tlv.FinalBlockId =     26;
Tlv.SignatureType =    27;
Tlv.KeyLocator =       28;
Tlv.KeyLocatorDigest = 29;
Tlv.SelectedDelegation = 32;
Tlv.FaceInstance =     128;
Tlv.ForwardingEntry =  129;
Tlv.StatusResponse =   130;
Tlv.Action =           131;
Tlv.FaceID =           132;
Tlv.IPProto =          133;
Tlv.Host =             134;
Tlv.Port =             135;
Tlv.MulticastInterface = 136;
Tlv.MulticastTTL =     137;
Tlv.ForwardingFlags =  138;
Tlv.StatusCode =       139;
Tlv.StatusText =       140;

Tlv.SignatureType_DigestSha256 = 0;
Tlv.SignatureType_SignatureSha256WithRsa = 1;
Tlv.SignatureType_SignatureSha256WithEcdsa = 3;
Tlv.SignatureType_SignatureHmacWithSha256 = 4;

Tlv.ContentType_Default = 0;
Tlv.ContentType_Link =    1;
Tlv.ContentType_Key =     2;

Tlv.NfdCommand_ControlResponse = 101;
Tlv.NfdCommand_StatusCode =      102;
Tlv.NfdCommand_StatusText =      103;

Tlv.ControlParameters_ControlParameters =   104;
Tlv.ControlParameters_FaceId =              105;
Tlv.ControlParameters_Uri =                 114;
Tlv.ControlParameters_LocalControlFeature = 110;
Tlv.ControlParameters_Origin =              111;
Tlv.ControlParameters_Cost =                106;
Tlv.ControlParameters_Flags =               108;
Tlv.ControlParameters_Strategy =            107;
Tlv.ControlParameters_ExpirationPeriod =    109;

Tlv.LocalControlHeader_LocalControlHeader = 80;
Tlv.LocalControlHeader_IncomingFaceId = 81;
Tlv.LocalControlHeader_NextHopFaceId = 82;
Tlv.LocalControlHeader_CachingPolicy = 83;
Tlv.LocalControlHeader_NoCache = 96;

Tlv.Link_Preference = 30;
Tlv.Link_Delegation = 31;

Tlv.Encrypt_EncryptedContent = 130;
Tlv.Encrypt_EncryptionAlgorithm = 131;
Tlv.Encrypt_EncryptedPayload = 132;
Tlv.Encrypt_InitialVector = 133;

// For RepetitiveInterval.
Tlv.Encrypt_StartDate = 134;
Tlv.Encrypt_EndDate = 135;
Tlv.Encrypt_IntervalStartHour = 136;
Tlv.Encrypt_IntervalEndHour = 137;
Tlv.Encrypt_NRepeats = 138;
Tlv.Encrypt_RepeatUnit = 139;
Tlv.Encrypt_RepetitiveInterval = 140;

// For Schedule.
Tlv.Encrypt_WhiteIntervalList = 141;
Tlv.Encrypt_BlackIntervalList = 142;
Tlv.Encrypt_Schedule = 143;

/**
 * Strip off the lower 32 bits of x and divide by 2^32, returning the "high
 * bytes" above 32 bits.  This is necessary because JavaScript << and >> are
 * restricted to 32 bits.
 * (This could be a general function, but we define it here so that the
 * Tlv encoder/decoder is self-contained.)
 * @param {number} x
 * @returns {number}
 */
Tlv.getHighBytes = function(x)
{
  // Don't use floor because we expect the caller to use & and >> on the result
  // which already strip off the fraction.
  return (x - (x % 0x100000000)) / 0x100000000;
};
