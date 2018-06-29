#!/bin/bash

cd /program2vec
mkdir -p output

echo "Generating path-based walks"
python2 -m walker walk --bitcode data/vmlinux.o.bc --output output/pathbased.walks --length 100 --walks 100 --interprocedural 1 --bias 2.0 --remove-cross-folder

echo "Generating flat walks"
python2 -m walker walk --bitcode data/vmlinux.o.bc --output output/flat.walks --length 100 --walks 100 --interprocedural 0 --flat

echo "Training path-based model"
python2 -m walker train --input output/pathbased.walks --output output/pathbased.model --mincount 5 --window 1

echo "Training flat model"
python2 -m walker train --input output/flat.walks --output output/flat.model --mincount 5 --window 1

echo "Path-based: evaluating manual synonyms with AUROC"
echo "ROC curve output to pathbased-manual-roc.png"
python2 -m walker metric roc --model output/pathbased.model --reference data/manual-synonyms.txt --png output/pathbased-manual-roc.png | tee

echo "Path-based: evaluating underscore synonyms with AUROC"
echo "ROC curve output to pathbased-underscores-roc.png"
echo "(Table 1 in the paper)"
python2 -m walker metric roc --model output/pathbased.model --reference data/underscores.txt --png output/pathbased-underscores-roc.png

echo "Flat: evaluating manual synonyms with AUROC"
echo "ROC curve output to flat-underscores-roc.png"
echo "(Table 1 in the paper)"
python2 -m walker metric roc --model output/flat.model --reference data/manual-synonyms.txt --png output/flat-manual-roc.png

echo "Flat: evaluating underscore synonyms with AUROC"
echo "ROC curve output to flat-underscores-roc.png"
echo "(Table 1 in the paper)"
python2 -m walker metric roc --model output/flat.model --reference data/underscores.txt --png output/flat-underscores.roc.png

echo "Clustering the path-based model with k-Means"
python2 -m walker cluster kmeans --k 3500 --model output/pathbased.model --output output/pathbased-clusters.txt

echo "Clustering the flat model with k-Means"
python2 -m walker cluster kmeans --k 3500 --model output/flat.model --output output/flat-clusters.txt

echo "Path-based: evaluating manual synonyms with cluster f1"
echo "(Table 1 in the paper)"
python2 -m walker metric f1cluster --reference data/manual-synonyms.txt --clusters output/pathbased-clusters.txt

echo "Flat: evaluating manual synonyms with cluster f1"
echo "(Table 1 in the paper)"
python2 -m walker metric f1cluster --reference data/manual-synonyms.txt --clusters output/flat-clusters.txt

echo "Path-based: evaluating underscore synonyms with cluster f1"
echo "(Table 1 in the paper)"
python2 -m walker metric f1cluster --reference data/underscores.txt --clusters output/pathbased-clusters.txt

echo "Flat: evaluating underscore synonyms with cluster f1"
echo "(Table 1 in the paper)"
python2 -m walker metric f1cluster --reference data/underscores.txt --clusters output/flat-clusters.txt

echo "Running the mining evaluation (runminer.py)"
python2 runminer.py
