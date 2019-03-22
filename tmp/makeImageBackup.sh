cd /home/guille/Documentos/Fuentes
DATE=`date +"%Y%m%d_%H%M%S"`
cp huemul/image/huemul.image backup/huemul.$DATE.image
gzip backup/huemul.$DATE.image
