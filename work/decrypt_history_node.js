const fs = require("fs");
const crypto = require("crypto");
const zlib = require("zlib");

const [metaPath, blobPath, outPath] = process.argv.slice(2);
const meta = JSON.parse(fs.readFileSync(metaPath, "utf8"));
const data = fs.readFileSync(blobPath);

const tag = data.subarray(data.length - 16);
const encrypted = data.subarray(0, data.length - 16);
const decipher = crypto.createDecipheriv(
  "aes-128-gcm",
  Buffer.from(meta.key, "hex"),
  Buffer.from(meta.uuid, "hex"),
);
decipher.setAuthTag(tag);
const compressed = Buffer.concat([decipher.update(encrypted), decipher.final()]);
const text = zlib.gunzipSync(compressed).toString("utf8");

let output = text;
try {
  output = JSON.stringify(JSON.parse(text), null, 2);
} catch {}

fs.writeFileSync(outPath, output, "utf8");
console.log(`decrypted ${text.length} chars to ${outPath}`);
console.log(text.slice(0, 500));
