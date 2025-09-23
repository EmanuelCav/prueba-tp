#!/bin/bash

# Script para ejecutar todo el sistema distribuido
# Autor: Sistema de pruebas automático

set -e  # Salir si hay algún error

echo "=========================================="
echo "  EJECUTANDO SISTEMA DISTRIBUIDO "
echo "=========================================="

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Función para mostrar mensajes
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_debug() {
    echo -e "${BLUE}[DEBUG]${NC} $1"
}

# Verificar que estamos en el directorio correcto
if [ ! -f "README.md" ]; then
    log_error "Este script debe ejecutarse desde el directorio raíz del proyecto"
    exit 1
fi

# Verificar que los ejecutables existen
check_executable() {
    if [ ! -f "$1" ]; then
        log_error "No se encontró el ejecutable: $1"
        log_info "Ejecuta primero: ./build_all.sh"
        exit 1
    fi
}

log_info "Verificando ejecutables..."
check_executable "storage/bin/storage"
check_executable "master/bin/master"
check_executable "worker/bin/worker"
check_executable "query_control/bin/query_control"

# Función para limpiar procesos al salir
cleanup() {
    log_info "Cerrando procesos..."
    pkill -f "storage/bin/storage" 2>/dev/null || true
    pkill -f "master/bin/master" 2>/dev/null || true
    pkill -f "worker/bin/worker" 2>/dev/null || true
    pkill -f "query_control/bin/query_control" 2>/dev/null || true
    exit 0
}

# Capturar señales para limpiar procesos
trap cleanup SIGINT SIGTERM

log_info "Iniciando módulos del sistema..."

# 1. Iniciar Storage 
log_info "Iniciando Storage..."
cd storage
./bin/storage config/storage.config 2>&1 &
STORAGE_PID=$!
cd ..
sleep 2

# Verificar que storage está corriendo
if ! kill -0 $STORAGE_PID 2>/dev/null; then
    log_error "Storage no pudo iniciarse. Revisa logs/storage.log"
    exit 1
fi
log_info "✓ Storage iniciado (PID: $STORAGE_PID)"

# 2. Iniciar Master
log_info "Iniciando Master..."
cd master
./bin/master config/master.config 2>&1 &
MASTER_PID=$!
cd ..
sleep 2

# Verificar que master está corriendo
if ! kill -0 $MASTER_PID 2>/dev/null; then
    log_error "Master no pudo iniciarse. Revisa logs/master.log"
    cleanup
    exit 1
fi
log_info "✓ Master iniciado (PID: $MASTER_PID)"

# 3. Iniciar Worker
log_info "Iniciando Worker..."
cd worker
./bin/worker config/worker.config > ../logs/worker.log 2>&1 &
WORKER_PID=$!
cd ..
sleep 2

# Verificar que worker está corriendo
if ! kill -0 $WORKER_PID 2>/dev/null; then
    log_error "Worker no pudo iniciarse. Revisa logs/worker.log"
    cleanup
    exit 1
fi
log_info "✓ Worker iniciado (PID: $WORKER_PID)"


echo ""
echo "=========================================="
log_info "¡SISTEMA INICIADO EXITOSAMENTE!"
echo "=========================================="
echo ""
echo "Procesos activos:"
echo "  - Storage (PID: $STORAGE_PID) - Puerto 9002"
echo "  - Master (PID: $MASTER_PID) - Puerto 9001"
echo "  - Worker (PID: $WORKER_PID)"
echo ""
echo "Logs disponibles en:"
echo "  - logs/storage.log"
echo "  - logs/master.log"
echo "  - logs/worker.log"
echo ""
echo "Para probar el sistema, ejecuta en otra terminal:"
echo "  ./test.sh"

# Mantener el script corriendo
while true; do
    sleep 1
    # Verificar que todos los procesos siguen corriendo
    if ! kill -0 $STORAGE_PID 2>/dev/null; then
        log_error "Storage se cerró inesperadamente"
        cleanup
        exit 1
    fi
    if ! kill -0 $MASTER_PID 2>/dev/null; then
        log_error "Master se cerró inesperadamente"
        cleanup
        exit 1
    fi
    if ! kill -0 $WORKER_PID 2>/dev/null; then
        log_error "Worker se cerró inesperadamente"
        cleanup
        exit 1
    fi
done
