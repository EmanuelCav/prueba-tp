#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}


log_info "Verificando conexiones..."

# Verificar puertos
if nc -z 127.0.0.1 9002 2>/dev/null; then
    log_success "Storage escuchando en puerto 9002"
else
    log_error "Storage no está escuchando"
fi

if nc -z 127.0.0.1 9001 2>/dev/null; then
    log_success "Master escuchando en puerto 9001"
else
    log_error "Master no está escuchando"
fi

# Verificar Worker conectado
sleep 1
if grep -q "Worker conectado" master/logs/master.log 2>/dev/null; then
    log_success "Worker conectado al Master"
else
    log_error "Worker no detectado en logs del Master"
fi

cd ..

# 7. Verificar comunicación Storage
if grep -q "Worker solicitó tamaño" storage/logs/storage.log 2>/dev/null; then
    log_success "Worker se comunicó con Storage"
else
    log_error "No se detectó comunicación Worker->Storage"
fi

# 8. Limpiar
log_info "Deteniendo módulos..."
kill $STORAGE_PID $MASTER_PID $WORKER_PID 2>/dev/null
sleep 1
