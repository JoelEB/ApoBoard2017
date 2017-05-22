#Python 2.7
#gene database generator
#makes file: gene_db.txt

MaxEffects = 14
MaxColorsets = 8
GeneNum = 0

outfile = open("gene_db.txt","w")

for colorset in range(MaxColorsets):
    for effect in range(MaxEffects):
        gene = (effect << 8) + colorset
        outfile.write(str(GeneNum) + ": " + hex(0x10000 + gene)[-4:]+"\n")
        GeneNum += 1
outfile.close()
print("Wrote db of " + str(GeneNum-1)+" genes.")

        
    