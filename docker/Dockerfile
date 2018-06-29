FROM ubuntu:xenial

RUN apt-get update -y && apt-get install -y ansible time

COPY ansible /root/ansible
COPY fetch_and_run.sh /usr/local/bin
COPY github_deploy_key /root/.ssh/id_rsa
RUN chmod 400 /root/.ssh/id_rsa
COPY github_deploy_key.pub /root/.ssh/id_rsa.pub
COPY known_hosts /root/.ssh/known_hosts
RUN ansible-playbook /root/ansible/aws.yml

RUN apt-get update -y && apt-get install -y sqlite3 vim-nox

RUN echo 'export PYTHONPATH="/program2vec/src"' >> /root/.bashrc 
RUN echo 'export LD_LIBRARY_PATH="/program2vec/build"' >> /root/.bashrc 
