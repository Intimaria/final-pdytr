# Setup del experimento distribuido ping-pong con CPU Load 

Este documento explica paso a paso cómo configurar rápidamente tu entorno para ejecutar el experimento distribuido usando Ansible. Las instruccioneses asumen un entorno de Linux con bash. 

*NOTA: Este setup asume que existen dos PCs en una LAN cableada. En el setup actual, el server usa Debian y el cliente corre SuSE. Modificar acorde a su setup.*



## Estructura del Proyecto

```plaintext
├── inventory.yaml
├── playbooks
│   ├── setup.yml
│   └── sync.yml
├── README.md
└── roles
    ├── setup-experiment
    │   ├── files
    │   │   ├── checksum.c
    │   │   ├── checksum.h
    │   │   ├── client.c
    │   │   ├── client-run.sh
    │   │   ├── server.c
    │   │   └── server-run.sh
    │   ├── meta
    │   │   └── main.yml
    │   ├── tasks
    │   │   └── main.yml
    │   └── vars
    │       └── main.yml
    └── synchronization-setup
        ├── handlers
        │   └── main.yml
        ├── tasks
        │   ├── configure_client.yml
        │   ├── configure_server.yml
        │   ├── install.yml
        │   └── main.yml
        └── templates
            ├── chrony_client.conf.j2
            └── chrony_server.conf.j2
```
**Descripción:**
- `inventory.yaml`: define los hosts del experimento.
- `playbooks/`: contiene los playbooks principales de Ansible (`setup.yml`, `sync.yml`).
- `roles/`:
  - `setup-experiment/`: código del experimento en C y scripts de ejecución.
  - `synchronization-setup/`: configuración del servicio de sincronización de hora (Chrony).

El código del experimento (`client.c`, `server.c`) y sus utilidades (`checksum.c`, `*.sh`) se encuentran en `roles/setup-experiment/files/`.

---

## Conexión LAN

Se requiere que las dos máquinas estén conectadas por cable Ethernet a un mismo switch o router.
- Idealmente configurar IPs estáticas o comprobar que ambas estén en la misma red.
- Comprobá conectividad con:

```bash
ping <ip_del_otro_host>
```

---
##  Instalar direnv

`direnv` maneja automáticamente variables de entorno:

```bash
curl -sfL https://direnv.net/install.sh | bash

echo 'eval "$(direnv hook bash)"' >> ~/.bashrc
source ~/.bashrc
```

##  Instalar asdf

`asdf` gestiona versiones de múltiples lenguajes (incluyendo Python):

```bash
mkdir -p ~/.asdf/bin
curl -L "https://github.com/asdf-vm/asdf/releases/download/v0.16.7/asdf-v0.16.7-$(uname | tr '[:upper:]' '[:lower:]')-amd64.tar.gz" | tar -xz -C ~/.asdf/bin

echo 'export PATH="$HOME/.asdf/bin:$PATH"' >> ~/.bashrc
echo 'export ASDF_DIR="$HOME/.asdf"' >> ~/.bashrc
source ~/.bashrc
```

##  Instalar Python con asdf

```bash
asdf plugin add python
asdf install python 3.11.11
```


## Clonar el Repositorio

```bash
git clone git@github.com:Intimaria/final-pdytr.git
cd final-pdytr
```


## Ambiente virtual Python 

Al entrar en el directorio debes correr `direnv allow`, esto generará un ambiente virtual en el directorio.

##  Instalar Ansible

Con Python configurado, instalá Ansible:

```bash
pip install ansible
```

---

## Configuración del Inventario

Editá el archivo `inventory.yml` con las IPs correctas de tus máquinas:

```yaml
all:
  children:
    ntp_server:
      hosts:
        ping:
          ansible_host: <ip server>
          ansible_user: tu_usuario
          ansible_become: true
    ntp_client:
      hosts:
        pong:
          ansible_host: <ip cliente>>
          ansible_user: tu_usuario
          ansible_become: true
```

**Nota:** Si no conocés la IP, ejecutá:

```bash
ip a
```

y reemplazá en el archivo anterior.

---

## Ejecutar los Playbooks con Ansible

Desde la carpeta del proyecto, ejecutá en orden:

```bash
ansible-playbook -i inventory.yml playbooks/setup_experiment.yml
ansible-playbook -i inventory.yml playbooks/synchronization.yml
```

---

## Ejecutar los scripts 

Si querés correr los scripts manualmente, conectate a cada máquina ajustando los valores acorde:

### En el servidor:
```bash
cd /tmp/experiment
export PORT=5000
export BUFFER_SIZE=102400
export CPU_LOAD=1
./server-run.sh 
```

### En el cliente:
```bash
cd /tmp/experiment
export SERVER_IP=<ip server>
export PORT=5000
export BUFFER_SIZE=102400
export CPU_LOAD=1
./client-run.sh
```

---

### Nota técnica sobre el parámetro de Clock Offset

No es necesario pasar el offset manualmente. El script `client-run.sh` obtiene automáticamente el valor de offset ejecutando:

```bash
chronyc tracking | awk '/Last offset/ {print $4}'
```

El valor se imprime en consola para registrar las estadísticas. Si examinás el código en C, verás que aunque el valor de offset es recibido como argumento, no se aplica directamente en el código para mantener disponible el registro crudo del valor original del offset.

**Ejemplo de salida típica:**

```plaintext
Starting client: SERVER_IP=192.168.0.33, PORT=5000, CLOCK_OFFSET=+0.000027817, CPU_LOAD=1
Client→Server One-Way Delay: 0.001436 seconds
Server→Client One-Way Delay: 0.000437 seconds
Clock offset: +0.000027817
Client experiments completed.
```

**Uso en análisis:**

En el experimento se usaronn los valores de Clock Offset, Delay ida y Delay vuelta en una planilla de Google Sheets para calcular promedios, desviaciones estándar, y graficar las diferencias entre experimentos.