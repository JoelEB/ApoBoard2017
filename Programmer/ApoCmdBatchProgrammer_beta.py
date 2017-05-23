from subprocess import Popen, PIPE
import threading
import fcntl
import os
import Queue
import cmd
from random import *

""" ApoBadgeV03
#define eeprom_NumGenes 0
#define eeprom_CRC16    1 //16-bit checksum of genes
#define eeprom_genes_start 10
"""
eeprom_NumGenes    =0
eeprom_CRC16       =1 #16-bit checksum of genes
eeprom_genes_start =10
eeprom_badgeNum    =128 #try to keep badgeNum outside of gene area

class BadgeCmd(cmd.Cmd):
    prompt = '\nApoCmd: '
    intro =  "ApoBadge2017 auto-programmer.  SPECTER v.01 build5.19.17" 


    def do_write_gene(self, line):
        lsplit = line.split("=")
        if len(lsplit) != 2 or line=="":
            print("syntax: Gene#=0xEffectColorset  example: 0=0D07 sets 0 to effect D(13) and colorset 7")
            return False
        p = avd_start_interactive()
        if p == False:
                return False

        genenum = int(lsplit[0],16)
        genecode = int(lsplit[1],16)
        address = eeprom_genes_start + genenum * 2
        avd_cmd = "write eeprom "+hex(address)+" "+hex(genecode>>8) + " "+hex(genecode &0xff)+"\n"
        print(avd_cmd)
        p.stdin.write(avd_cmd)        
        avd_quit(p)

        return False
    
    #does not write eeprom
    def do_write_flash(self,line): #if not given a cmd then uses Apo*hex
        
        if(check_m328_connected() == False):
            print("No ATMEGA328 connected")
            return False


        #clear HFUSE bit 3 (EEPROMsave) so EEPROM doesn't get wiped by ERASE
        p = avd_start_interactive()
        if p == False:
                return False
        change_fuse_saveEEPROM(p)
        avd_quit(p)
    
        invokecmd =  "avrdude avrdude -p m328 -c avrispmkii -V -F "
        invokecmd += "-Uflash:w:"
        invokecmd += hexfile
        invokecmd += ":i -B 1" 
        """
        options:
        -p m328
        -c avrispmkii
        -D    ;disable auto erase
        -F    ;force connection is device # unrecognized (which it always is)
        -U    ;write memory with HARDCODED filename (will fix soon)
        -V    ;disable verify -- waste of time
        -q not used;quells progress bar
        -B    ;bitclock speed 10 works @ 9.83s, 1 doesn't work
        """
        print("Attempting to write flash")
        p = Popen(invokecmd,  shell=True, stdin=PIPE, stdout=PIPE, stderr=PIPE)
        avd_reply = str(p.communicate())
        if avd_reply.find("32768 bytes of flash written")>0:
            print("Success.")
        else:
            print("Error writing to flash")
            print(avd_reply)
            
    def do_write_batch_genes(self,line):
        
        genedb = open("gene_db.txt","r")
        
        if line=="":
            badgeNum=1
        else:
            badgeNum = int(line)
        while True:
            geneln = genedb.readline().split(":")
            gene_list = geneln[1]
            gene_list = gene_list.strip()
            if gene_list[-1:] == "\n":
                gene_list = gene_list[:-1] #trim newline of end
            geneNumDb = str(geneln[0])
            gene = int(gene_list,16)
            user_input = raw_input("\nFlash and write EEPROM as badge #"+str(badgeNum)+"? enter 'y' to continue")
            if user_input[0] == "Y" or user_input[0] =="y":
                print("Writing EEPROM for badge #" + str(badgeNum) + " GENE: "+ hex(0x10000+gene)[-4:])
                if(self.do_write_genes(gene_list) == False):
                    p = avd_start_interactive()
                    if p == False:
                        return False
                    write_badgeNum(badgeNum)
                    avd_quit(p)
                    badgeNum += 1
            else:
                print("Exiting batch gene programmer")
                break

    def do_write_batch_flash(self,line):
        genedb = open("gene_db.txt","r")
        
        if line=="":
            badgeNum=1
        else:
            badgeNum = int(line)
        while True:
            geneln = genedb.readline().split(":")
            gene_list = geneln[1]
            gene_list = gene_list.strip()
            if gene_list[-1:] == "\n":
                gene_list = gene_list[:-1] #trim newline of end
            geneNumDb = str(geneln[0])
            gene = int(gene_list,16)
            user_input = raw_input("\nFlash and write EEPROM as badge #"+str(badgeNum)+"? enter 'y' to continue")
            if user_input[0] == "Y" or user_input[0] =="y":
                print("Writing EEPROM for badge #" + str(badgeNum) + " GENE: "+ hex(0x10000+gene)[-4:])
                if(self.do_write_genes(gene_list) == False):
                    p = avd_start_interactive()
                    if p == False:
                        return False
                    write_badgeNum(badgeNum, p)
                    avd_quit(p)
                    badgeNum += 1
                    self.do_write_flash("")
            else:
                print("Exiting batch flash+gene programmer")
                break

    def do_write_genes(self, line):        
        lsplit = line.split(",")
        if lsplit == 0 or line=="":
            print("syntax: comma seperated list of genes  example: 0d07,0a10,0b02")
            return False
        p = avd_start_interactive()
        if p == False:
                return False
        NumGenes = len(lsplit)
        for gene in range(NumGenes):
            genecode = int(lsplit[gene],16)
            eeprom_shadow[gene*2 + eeprom_genes_start] = (genecode >> 8) & 0xFF
            eeprom_shadow[gene*2 + eeprom_genes_start + 1] = genecode & 0xFF
        eeprom_shadow[eeprom_NumGenes] = NumGenes
        gene_checksum = geneCRC16(NumGenes)
        eeprom_shadow[eeprom_CRC16] = (gene_checksum >> 8) & 0xff
        eeprom_shadow[eeprom_CRC16 + 1] = gene_checksum & 0xff

        
        write_eeprom_shadow(eeprom_genes_start + NumGenes*2, p) #takes: total bytes to write from eeprom_shadow to eeprom, p = subprocess.popen object                      
        avd_quit(p)
        
        return False #False = OK
    
    def do_read_genes(self,line):
        p = avd_start_interactive()
        if p == False:
                return False
    
        read_eeprom_into_shadow(p)
        avd_quit(p)
        NumGenes = eeprom_shadow[eeprom_NumGenes]
        if NumGenes > 58:
            NumGenes = 58
            print("NumGenes clipped to 58")
        gene_checksum = (eeprom_shadow[eeprom_CRC16]<<8) + eeprom_shadow[eeprom_CRC16 + 1]
        print("\nNumgenes = "+str(NumGenes))
        for gene in range(NumGenes):
            eeprom_gene = (eeprom_shadow[gene*2 + eeprom_genes_start] << 8) + eeprom_shadow[gene*2 + eeprom_genes_start + 1]
            print(str(gene)+": "+hex(0x10000+eeprom_gene)[-4:])
        if geneCRC16(NumGenes) == gene_checksum:
                  print("Checksum ok: "+hex(gene_checksum))
        else:
                  print("BAD checksum: "+hex(gene_checksum))
        return False

    def do_exit(self, line):
        return True
    
    def do_quit(self, line):
        return True
        
    def do_EOF(self, line):
        return True

def check_m328_connected():
    avd_cmd = "avrdude -c avrispmkii -p m328p -P usb"
    p = Popen(avd_cmd,  shell=True, stdin=PIPE, stdout=PIPE, stderr=PIPE)
    avd_reply = str(p.communicate())
    if (avd_reply.find("failed") >= 0):
        return False
    return True
    
def read_eeprom_into_shadow(p):
    print("reading EEPROM")
    avd_cmd = "read eeprom 0x00 0x80\n"
    p.stdin.write(avd_cmd)
    avd_reply = p.stdout.readline().split(" ")
    if avd_reply[0] != "\n":
        print("unhandled reply from avd 'read eeprom' line 1")
        return False
    avd_reply = p.stdout.readline().split(" ")
    if avd_reply[0] != "avrdude>":
        print("unhandled reply from avd 'read eeprom' line 2")
        return False
    avd_reply = p.stdout.readline().split(" ")
    if avd_reply[0] != ">>>":
        print("unhandled reply from avd 'read eeprom' line 3")
        return False
    for addr_counter in range(0x00,0x80,0x10):
        avd_reply = p.stdout.readline().split(" ")
        if len(avd_reply) < 21:
            print("unhandled replay from avd 'read eeprom':")
            print(avd_reply)
            return False
        addr = int(avd_reply[0],16)
        if addr != addr_counter:
            print("unexpected address from avd 'read eeprom': "+hex(addr))
            return False
        for avd_byte in range(2,10)+range(11,19):
            eeprom_shadow[addr] = avd_reply[avd_byte].decode("hex")
            addr += 1
    return True
    
def change_fuse_saveEEPROM(p): #assumes terminal mode
    avd_cmd = "read hfuse\n"
    p.stdin.write(avd_cmd)
    p.stdout.readline()
    p.stdout.readline()
    p.stdout.readline()
    avr_reply  = p.stdout.readline().split(' ')
    if len(avr_reply) < 2:
        print("read hfuse bad avd response")
        return False
    hfuse = int(avr_reply[2],16)
    print("hfuse = " + hex(hfuse))
    if hfuse & 0x08:
        print ("clearing hfuse EEPROMsave bit")
        hfuse &= 0xF7
        print ("changing to "+hex(hfuse))
        avd_cmd = "write hfuse 0x00 "+hex(hfuse)+"\n"
        p.stdin.write(avd_cmd)
        p.stdout.readline()
        p.stdout.readline()
        if p.stdout.readline().find("write hfuse 0x00") >0:
            return True
        else:
            print ("unexpected response from avd write hfuse")
            return False
        
    else:
        print ("hfuse EEPROMsave already cleared")

    
def write_badgeNum(badgeNum, p): #assumes terminal mode
    avd_cmd = "write eeprom " + hex(eeprom_badgeNum) + " " + str(badgeNum) +"\n"
    p.stdin.write(avd_cmd)
    print("setting badgeNum to " + str(badgeNum))    
def write_eeprom_shadow(numbytes, p):
    adr = 0
    print("Writing " + str(numbytes) + " bytes to EEPROM")
    while (adr < numbytes):
        avd_cmd = "write eeprom "
        avd_bytecount = 0
        avd_cmd += hex(adr)
        while(avd_bytecount < 16 and adr < numbytes):
            avd_cmd += " "+hex(eeprom_shadow[adr])
            avd_bytecount += 1
            adr += 1
              
        avd_cmd += "\n"
        #print("write cmd to avd:")
        #print(avd_cmd)
        p.stdin.write(avd_cmd)
        p.stdout.readline()
        p.stdout.readline()
        if(p.stdout.readline().find("write eeprom")>0):
            print("EEPROM write OK")
        else:
            print("EEPROM write failed @"+hex(adr - avd_bytecount))
    return False # False = OK!

def geneCRC16(count): #count in _words_
    crc = 0
    address =  eeprom_genes_start
    while count > 0:
        count -= 1
        crc ^= (eeprom_shadow[address] << 8) + eeprom_shadow[address + 1]
        address += 2
        for i in range(8):
            if (crc & 1):
                crc = (crc >> 1) ^ 0xA001
            else:
                crc = (crc >> 1)
    return crc

def avd_quit(p):
    avd_cmd = "quit\n"
    print("Exiting Avrdude")
    p.stdin.write(avd_cmd)
    p.communicate()

def avd_start_interactive():
    p = Popen("avrdude -c avrispmkii -p m328p -P usb -F -t", shell=True, stdin=PIPE, stdout=PIPE, stderr=PIPE)
    p.stderr.readline()
    avd_reply = p.stderr.readline() 
    if( avd_reply.find("failed") > 0 ):
        print("No ATMEGA328 found")
        return False
    if ( avd_reply.find("done") > 0):
        print("No AVRispMKII found")
        return False
    #setNonBlocking(p.stdout)
    #setNonBlocking(p.stderr)

    p.stdin.write("sig\n") #get device signature
    p.stdout.readline()
    p.stdout.readline()
    avd_sig_reply = p.stdout.readline().split('=')[1][1:-1]
    

    if (avd_sig_reply == '0xffffff'):
        print("Failed to connect to badge")
        return False
    else:
        print("Avrdude connection opened to badge " + avd_sig_reply)
        return p
    
"""
INTEL HEX file format
A record (line of text) consists of six fields (parts) that appear in order from left to right:

    Start code, one character, an ASCII colon ':'.
    Byte count, two hex digits, indicating the number of bytes (hex digit pairs) in the data field. The maximum byte count is 255 (0xFF). 16 (0x10) and 32 (0x20) are commonly used byte counts.
    Address, four hex digits, representing the 16-bit beginning memory address offset of the data. The physical address of the data is computed by adding this offset to a previously established base address, thus allowing memory addressing beyond the 64 kilobyte limit of 16-bit addresses. The base address, which defaults to zero, can be changed by various types of records. Base addresses and address offsets are always expressed as big endian values.
    Record type (see record types below), two hex digits, 00 to 05, defining the meaning of the data field.
    Data, a sequence of n bytes of data, represented by 2n hex digits. Some records omit this field (n equals zero). The meaning and interpretation of data bytes depends on the application.
    Checksum, two hex digits, a computed value that can be used to verify the record has no errors.
"""
def calc_ihex_checksum(startaddress,bytelist):
    csum = len(bytelist)
    csum += (startaddress >> 8) & 0xFF
    csum += startaddress & 0xFF
    
    for getbyte in range(len(bytelist)):
        csum += bytelist[getbyte]
    print(csum)
    csum = csum & 0xFF
    csum = csum ^ 0xFF
    csum += 1
    
    csum = csum % 0xFF
    return csum
    
def fab_ihexline(startaddress,bytelist):
    out = ":" #Start code
    lb = len(bytelist)
    if startaddress < 0 or startaddress > 0x1fff or lb == 0 or lb > 255:
        return False
    out += hex(lb+0x100)[-2:] #Byte count ... the same wacky string shit that I luv python for
    cs = lb #checksum init
    out += hex(startaddress+0x10000)[-4:] #Address
    cs += (startaddress >> 8) & 0xFF
    cs += startaddress & 0xFF
    out += "00" #Record type 00 - 8bit data
    for getbyte in range(lb):
        out += hex(bytelist[getbyte]+0x100)[-2:] #Data
        cs += bytelist[getbyte]
    cs &= 0xFF
    cs ^= 0xFF
    cs += 1
    cs %= 0xFF
    out += hex(cs+0x100)[-2:] #iHEX checksum
    out += "\n"
    return(out)

def setNonBlocking(fd):
    """
    Set the file description of the given file descriptor to non-blocking.
    """
    flags = fcntl.fcntl(fd, fcntl.F_GETFL)
    flags = flags | os.O_NONBLOCK
    fcntl.fcntl(fd, fcntl.F_SETFL, flags)

def reader(fd, queue):
    while True:
        try:
            data = fd.read()
            queue.put(data)
        except IOError:
            continue
        except Exception:
            return

def flushAndWrite(msg, p):
    p.stdout.flush()
    p.stderr.flush()
    p.stdin.write(msg)
    p.stdin.flush()

def writeAndReadResponse(msg, infd, outfd):
    outfd.flush()
    infd.write(msg)
    infd.flush()
    while True:
        try:
            return outfd.read()
        except IOError:
            continue
        except KeyboardInterrupt:
            return ''


def getVtarget(msg):
    lines = msg.split('\n')
    for line in lines:
        if line.startswith('Vtarget'):
            return float(line.split(':')[1][:-2])
    return 0.0
#more copypasta, this time from
## http://stackoverflow.com/questions/1724693/find-a-file-in-python#1724723
import os, fnmatch
def find(pattern, path):
    result = []
    for root, dirs, files in os.walk(path):
        for name in files:
            if fnmatch.fnmatch(name, pattern):
                result.append(os.path.join(root, name))
    return result


if __name__ == '__main__':
    hexfile = find('ApoBoard*.hex', '.')[0]
    
    eeprom_0xFF_chars = "FF" * 128 #blank chars
    eeprom_shadow = bytearray(eeprom_0xFF_chars.decode("hex")) # fill eeprom_shadow with 0xFF
    BadgeCmd().cmdloop()
    
    

    """ unused stuff from copypasta
    outqueue = Queue.Queue()
    errqueue = Queue.Queue()
    #
    outthread = threading.Thread(target=reader, args=(p.stdout, outqueue))
    errthread = threading.Thread(target=reader, args=(p.stderr, errqueue))
    #
    outthread.start()
    errthread.start()
    """
