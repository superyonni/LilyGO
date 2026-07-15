// Codec ChirpStack para LilyGO T-Echo (payload 12 bytes BE)
// Pegar en Device profile > LilyGO-PROF > Codec > Custom JavaScript
//
// Bytes 0-3: lat int32 × 1e7 (big-endian)
// Bytes 4-7: lon int32 × 1e7
// Bytes 8-9: alt int16 m
// Bytes 10-11: battery mV uint16
//
// IMPORTANTE: en JavaScript NO usar (b[i] << 24) sin >>> 0; corrompe coordenadas negativas.

function u32(b, i) {
  return (((b[i] & 0xff) << 24) | ((b[i + 1] & 0xff) << 16) | ((b[i + 2] & 0xff) << 8) | (b[i + 3] & 0xff)) >>> 0;
}

function s32(b, i) {
  var v = u32(b, i);
  return v > 0x7fffffff ? v - 0x100000000 : v;
}

function u16(b, i) {
  return (((b[i] & 0xff) << 8) | (b[i + 1] & 0xff)) >>> 0;
}

function s16(b, i) {
  var v = u16(b, i);
  return v > 0x7fff ? v - 0x10000 : v;
}

function decodeUplink(input) {
  var b = input.bytes;
  if (b.length < 12) {
    return { errors: ["payload < 12 bytes, got " + b.length] };
  }

  var lat = s32(b, 0) / 1e7;
  var lon = s32(b, 4) / 1e7;
  var alt = s16(b, 8);
  var batMv = u16(b, 10);

  return {
    data: {
      latitude: lat,
      longitude: lon,
      altitude_m: alt,
      battery_mv: batMv,
      battery_v: +(batMv / 1000).toFixed(2),
      gps_valid: !(lat === 0 && lon === 0)
    }
  };
}

function encodeDownlink(input) {
  return { bytes: [] };
}
