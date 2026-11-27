"""Shared helpers for the cx580-pro tools.

Locates the vendored third-party code (Sony-PMCA-RE, fwtool.py) and exposes the
few primitives the project needs: building updater payloads (FDAT) for an
arbitrary model id, and talking to the camera over USB.

Nothing in this module writes to the camera.
"""

import io
import os
import sys
import warnings

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VENDOR = os.path.join(ROOT, 'vendor')
PMCA_DIR = os.path.join(VENDOR, 'Sony-PMCA-RE')
FWTOOL_DIR = os.path.join(VENDOR, 'fwtool.py')
FDAT_DIR = os.path.join(PMCA_DIR, 'updatershell', 'fdat')

# The vendored code predates Python 3.12 and triggers SyntaxWarnings for
# regex escapes; they are harmless.
warnings.filterwarnings('ignore', category=SyntaxWarning)

for _p in (FWTOOL_DIR, PMCA_DIR):
    if _p not in sys.path:
        sys.path.insert(0, _p)

#: Architecture of the 2012 Handycam family (HDR-PJ590V, PJ760V, TD20, ...).
ARCH = 'CXD4132'

#: Size of the per-model part of an encrypted updater payload. The model and
#: region ids, the FDAT header crc and the first block checksum all live in
#: the first three AES blocks; everything after that is model independent.
HDR_SIZE = 0x30


def known_models(arch=ARCH):
    """Returns {name: header_path} for the payload headers shipped with PMCA."""
    d = os.path.join(FDAT_DIR, arch)
    return {f[:-4]: os.path.join(d, f) for f in sorted(os.listdir(d)) if f.endswith('.hdr')}


def vendor_payload(name, arch=ARCH):
    """Returns the encrypted updater-shell payload PMCA ships for `name`."""
    with open(known_models(arch)[name], 'rb') as h, open(os.path.join(FDAT_DIR, arch + '.dat'), 'rb') as p:
        return h.read() + p.read()


def decode_payload(data):
    """Decrypts an updater payload. Returns (arch, FdatFile, plain_bytes)."""
    from fwtool.sony import fdat
    arch, plain = fdat.decryptFdat(io.BytesIO(data))
    plain = io.BytesIO(plain.read())
    return arch, fdat.readFdat(plain), plain.getvalue()


def build_payload(model, region=0, base='HDR-PJ590V', arch=ARCH):
    """Builds the updater-shell payload for an arbitrary model id.

    The body (the cramfs holding libupdaterbody.so) is taken unchanged from
    the payload PMCA ships for `base`; only the model and region fields, the
    header crc and the first block checksum differ.
    """
    from fwtool.sony import fdat
    _, base_fdat, _ = decode_payload(vendor_payload(base, arch))

    plain = io.BytesIO()
    fdat.writeFdat(fdat.FdatFile(
        model=model,
        region=region,
        version=base_fdat.version,
        isAccessory=fdat.modelIsAccessory(model),
        firmware=io.BytesIO(base_fdat.firmware.read()),
        fs=io.BytesIO(base_fdat.fs.read()),
    ), plain)
    return fdat.encryptFdat(plain, arch).read()


def model_ids(arch=ARCH):
    """Returns {name: (model, region)} decoded from the shipped headers."""
    out = {}
    for name in known_models(arch):
        _, f, _ = decode_payload(vendor_payload(name, arch))
        out[name] = (f.model, f.region)
    return out
