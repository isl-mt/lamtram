lamtram: A toolkit for language and translation modeling using neural networks
==============================================================================
by [Graham Neubig](http://www.phontron.com)

If you have any trouble in install or usage of lamtram, please contact the 
[lamtram-users group](https://groups.google.com/forum/#!forum/lamtram-users) or file 
an issue on the [github page](http://github.com/neubig/lamtram). In addition to the
usage info below, you can also take a look at the [nmt-tips tutorial](http://github.com/neubig/nmt-tips)
on building strong neural machine translation systems.

Install/Citation
----------------

First, in terms of standard libraries, you must have autotools, libtool, and Boost. If
you are on Ubuntu/Debian linux, you can install them below:

    $ sudo apt-get install autotools libtool libboost-all

You must install Eigen and cnn separately. Follow the directions on the
[cnn page](http://github.com/clab/cnn), which also explain about installing Eigen.
Note that at the moment, lamtram only works with the "v2" branch of cnn, so please
use the v2 branch, which can be checked out by typing `git fetch origin v2` and
`git checkout v2` after cloning.

Once these two packages are installed, run the following commands, specifying the
correct paths for cnn and Eigen.

    $ autoreconf -i
    $ ./configure --with-cnn=/path/to/cnn --with-eigen=/path/to/eigen
    $ make

In the instructions below, you can see how to use lamtram to train and use language
models, translation models, or text classifiers.

Lamtram is licensed under the LGPL 2.1 and may be used freely according to this, or
any later versions of the license. If you want to use lamtram in your research and
we'd appreciate if you use the following reference:

    @misc{neubig15lamtram,
    	author = {Graham Neubig},
    	title = {lamtram: A Toolkit for Language and Translation Modeling using Neural Networks},
    	howpublished = {http://www.github.com/neubig/lamtram},
    	year = {2015}
    }

Language Models
---------------

### Training ###

To train a language model, you should prepare a training data set train.txt and
a development data set dev.txt. Before running training, you should decide a
vocabulary that you want to use, and replace all words outside of the vocabulary
with the symbol '<unk>'. The easiest way to do this is with the `script/unk-single.pl`
script.

    $ script/unk-single.pl < train.txt > train.unk

Then, we can perform training with `lamtram-train`. Here is a typical way to run it
with options:

    $ src/lamtram/lamtram-train \
        --model_type nlm \        # Train a neural language model
        --context 2 \             # Use a 2-gram context
        --layers "lstm:100:1" \   # Create a single 100-node LSTM layer
        --trainer sgd \           # Use sgd for training models
        --learning_rate 0.1 \     # Set learning rate to 0.1
        --seed 0 \                # A random seed, or 0 for a different seed every run
        --train_trg train.unk \   # Specify the training file
        --dev_trg dev.txt \       # Specify the development file
        --model_out langmodel.out

Training will take a long time (at least until the "speed improvements" below are
finished), so try it out on a small data set first. As soon as one iteration
finishes, the model will be written out, so you can use the model right away.

### Evaluating Perplexity ###

You can measure perplexity on a separate test set `test.txt`

    $ src/lamtram/lamtram --operation ppl --models_in nlm=langmodel.out < test.txt

### Generating Sentences ###

Once you have a model, you can try randomly generating sentences from the model:

    $ src/lamtram/lamtram --sents 100 --operation gen --models_in nlm=langmodel.out

(Note: this may not work right now as of 2015-4-11. It will just generate the single
most likely sentence over and over again. This will be fixed soon.)

Translation Models
------------------

### Training ###

Training a translation model is similar to language models. Prepare a parallel corpus
consisting of train-src.txt and train-trg.txt (in the source and target languages), and
dev-src.txt and dev-trg.txt. First make '<unk>' symbols like before.

    $ script/unk-single.pl < train-src.txt > train-src.unk
    $ script/unk-single.pl < train-trg.txt > train-trg.unk

Then, we can perform training with `lamtram-train`. Here is a typical way to run it to train
an LSTM encoder-decoder model (as in Sutskever et. al NIPS2014). You can also train attentional
models (Bahdanau et al. ICLR2015) by just changing "encdec" into "encatt."

    $ src/lamtram/lamtram-train \
        --model_type encdec \     # Create an encoder-decoder model
        --context 2 \             # Use a 2-gram context
        --layers "lstm:100:1" \   # Create a single 100-node LSTM layer
        --trainer sgd \           # Use sgd for training models
        --learning_rate 0.1 \     # Set learning rate to 0.1
        --seed 0 \                # A random seed, or 0 for a different seed every run
        --train_src train-src.unk \ # Specify the training source file
        --train_trg train-trg.unk \ # Specify the training target file
        --dev_src dev-src.txt  \  # Specify the development source file
        --dev_trg dev-trg.txt \   # Specify the development target file
        --model_out transmodel.out

Again, as soon as one iteration finishes, the model will be written out.
      
### Evaluating Perplexity ###

You can measure perplexity on a separate test set `test-src.txt` and `test-trg.txt`

    $ src/lamtram/lamtram \
        --operation ppl \
        --src_in test-src.txt \
        --models_in encdec=transmodel.out \
        < test-trg.txt

If you used the same data is before, the perplexity of the translation model should
be significantly lower than that of the language model, as the translation model has
the extra information from the input sentence.

### Decoding ###

You can also generate the most likely translation for the input sentence. Here is a typical
use case.

    $ src/lamtram/lamtram \
        --operation gen \
        --models_in encdec=transmodel.out \
        --beam 5 \        # Perform beam search with a beam of 5
        --word_pen 0.0 \  # The word penalty can be used to increase or decrease
                          # the average length of the sentence (positive for more words)
        > results.txt

You can also use model ensembles. Model ensembles allow you to combine two different models
with different initializations or structures. Using model ensembles is as simple as listing
multiple models separated by a pipe.

    --models_in "encdec=transmodel.out|nlm=langmodel.out"

Note however that the models must have the same vocabulary (i.e. be trained on the same data).
Ensembles will work for both generation and perplexity measurement.

Classifiers
-----------

### Training ###

Training a text classifier is very similar to translation models. The only difference is that
you want to create a file of the text you want to classify train-src.txt, and a correct label 
file with one label per line, train-lbl.txt. This is not absolutely necessary, but you can
convert rare words in the source into unknown symbols like before:

    $ script/unk-single.pl < train-src.txt > train-src.unk

Then, we can perform training with `lamtram-train`. Here is a typical way to run it to train
an LSTM model.

    $ src/lamtram/lamtram-train \
        --model_type enccls \     # Create an encoder-classifier model
        --layers "lstm:100:1" \   # Create a single 100-node LSTM layer
        --trainer sgd \           # Use sgd for training models
        --learning_rate 0.1 \     # Set learning rate to 0.1
        --seed 0 \                # A random seed, or 0 for a different seed every run
        --train_src train-src.unk \ # Specify the training source file
        --train_trg train-lbl.txt \ # Specify the training label file
        --dev_src dev-src.txt \   # Specify the development source file
        --dev_trg dev-lbl.txt \   # Specify the development label file
        --model_out clsmodel.out

Again, as soon as one iteration finishes, the model will be written out.
      
### Evaluating Perplexity/Accuracy ###

You can measure perplexity and classification accuracy on a separate test set `test-src.txt`
and `test-lbl.txt`

    $ src/lamtram/lamtram \
        --operation clseval \
        --src_in test-src.txt \
        --models_in enccls=clsmodel.out \
        < test-lbl.txt

### Classifying ###

You can also generate the most likely label for the input sentence. Here is a typical use case.

    $ src/lamtram/lamtram \
        --operation cls \
        --models_in enccls=clsmodel.out \
        > results.txt

You can also use model ensembles as with the translation models.

    --models_in "enccls=clsmodel1.out|enccls=clsmodel2.out"

Note however that the models must have the same vocabulary (i.e. be trained on the same data).
Ensembles will work for both perplexity measurement and classification.

TODO
----

### Speed Improvements
* Noise-contrastive estimation
* Hierarchical softmax output
* Minibatch training using matrix/matrix multiplication

### Accuracy Improvements
* More update algorithms (rmsprop, adam, rprop)
* More activation functions (softplus, maxout)

### More Models
* Add other encoders

