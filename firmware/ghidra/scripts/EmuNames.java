// Emulates a "number -> name string" function for a range of numbers.
// Args: <outfile> <funcAddr> <first> <last>
// @category cx580
import ghidra.app.script.GhidraScript;
import ghidra.app.emulator.EmulatorHelper;
import ghidra.program.model.address.*;
import java.io.*;

public class EmuNames extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        Address f = toAddr(a[1]);
        long first = Long.decode(a[2]), last = Long.decode(a[3]);
        Address sentinel = toAddr(0x100L);
        int found = 0;
        // Jump tables: make sure everything after the entry is disassembled.
        long span = a.length > 4 ? Long.decode(a[4]) : 0x8000;
        for (long off = 0; off < span; off += 4) {
            Address x = f.add(off);
            if (currentProgram.getListing().getInstructionAt(x) == null && currentProgram.getListing().getDataAt(x) == null)
                disassemble(x);
        }
        try (PrintWriter w = new PrintWriter(new FileWriter(a[0]))) {
            for (long i = first; i <= last && !monitor.isCancelled(); i++) {
                EmulatorHelper emu = new EmulatorHelper(currentProgram);
                try {
                emu.writeRegister("sp", 0x7fff0000L);
                emu.writeRegister("lr", sentinel.getOffset());
                emu.writeRegister("r0", i);
                emu.writeRegister(emu.getPCRegister(), f.getOffset());
                int steps = 0;
                boolean ok = true;
                while (steps++ < 20000) {
                    if (emu.getExecutionAddress().equals(sentinel)) break;
                    if (!emu.step(monitor)) { ok = false; break; }
                }
                if (!ok || steps >= 20000) continue;
                long p = emu.readRegister("r0").longValue();
                if (p == 0) continue;
                Address pa = toAddr(p);
                if (!currentProgram.getMemory().contains(pa)) continue;
                StringBuilder sb = new StringBuilder();
                for (int k = 0; k < 96; k++) {
                    byte b = currentProgram.getMemory().getByte(pa.add(k));
                    if (b == 0) break;
                    sb.append((char) (b & 0xff));
                }
                if (sb.length() == 0) continue;
                w.println(String.format("%#x\t%s", i, sb));
                found++;
                } finally {
                    emu.dispose();
                }
            }
        }
        println("names: " + found);
    }
}
