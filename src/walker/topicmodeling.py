from gensim.models import KeyedVectors
from gensim.models.ldamodel import LdaModel

def lda(walks_list, numtopics):
    lda = LdaModel(walks_list, numtopics)

    return lda
