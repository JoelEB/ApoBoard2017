#Python 2.7
#gene database generator
#makes file: gene_db.txt

MaxEffects = 16
MaxColorsets = 16
GeneNum = 1 #start with Gene1

outfile = open("gene_db.txt","w")

#for colorset in range(1,MaxColorsets): #skip colorset 0
colorset = 1
effect = 0
while (GeneNum < (MaxEffects * MaxColorsets)):
#for effect in range(MaxEffects):
    gene = (effect << 8) + colorset
    outfile.write(str(GeneNum) + ": " + hex(0x10000 + gene)[-4:]+"\n")
    colorset += 1
    effect += 1
 
    if (colorset >= MaxColorsets):
        colorset = 1
    if (effect >= MaxEffects):
        effect = 0
    GeneNum += 1
outfile.close()
print("Wrote gene_db.txt of " + str(GeneNum-1)+" genes.")

        
    
