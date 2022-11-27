# Descrição:


# Esquema do programa

1. Setup

    - Definir variáveis de controle de chaves: master_key_count e invalid_tag_count
    - Declare system as unblocked
    - Init RFID byte array
    - Setup all pins

2. Infinite loop
    - Check if system is blocked
    - Check if new tag is detected

3. Tag detected
    - Check if is Master Tag



# Variáveis necesárias

arrays armazenados na memŕia ROM do chip, no caso de falta de energia:

array de bytes[4] x número de cartões válidos
array de bytes[4] x número de cartões inválidos
array de bytes[4] x numeor de cartoes bloqueados

array de bytes[4] -> chave_mestre

define CMD_PIN (botão de reset)
define BUZZER
define relé/transistor_PORTAO

define LEDINDICADOR (funções com botão reset)


# Funções

- conferirNúmeroCartão () - retorna array 4 do numero do cartão

- isValido(array cartão) retorna true se válido

- isInvalido(array cartão) retorna true se invalido

- invalidaCartão(array cartão para invalidar) retorna void: Adiciona o cartão à lista de cartões inválidos, confere se ação é salva e indica sinais sonoros

- bloqueiaCartão(array cartão para bloquear)

- validaCartão (array cartão para validar) retorna void: Adciona o cartão à lista de cartões inválidos, confere se ação é salva e indica sinais sonoros

- bloquearSistema() retorna void

- desbloquearSistema() retorna void

- abrePortão() retorna void

- sinalSonoro(código de sinal) retorna void

- sinalLuminoso(codigo de sinal) retorna void





# Descrição de funcionamento

- Passe cartão válido:
    - Abre o portão 
    
- Passar cartão inválido/desconhecido:
    - Manter portão fechado
    - Registrar cartão inválido (3 tentativas)
    
- Após 3 tentativas (mesmo cartão) VV
    - Cartão bloqueado
    - Sinal sonoro
    - Retorna
    
- Após 4+ tentativas(mesmo cartão) || Após 3 tentativas (cartões diferentes) VV
    - Sistema bloqueado (removendo chaves confiáveis)
    - Sinal Sonoro
    - Necessário reset físico de sistema
    
- Adicionar cartão  VV
    - Chave mestre (iniciar ação)
    - Cartão a ser adcionado (não está armazenado no banco de dados) VV
    - Chave mestre (salvar ação)
    - Sinal sonoro (confirmando o comando ou indicando falha)
    
- Remover cartão VV
    - Chave mestre (iniciar ação)
    - Cartão a ser removido (está armazenado no banco de dados de cartões confiáveis)
    - Chave mestre (salvar ação)
    - Sinal sonoro (confirmando o comando ou indicando falha)
    
- Bloquear sistema (sem remover chaves) VV
    - Chave mestre 3 vezes
    - Sinal sonoro (diferente)
    - Chave mestre 1 vez
    
- Desbloquear cartão (o cartão deixa de estar salvo no banco de dados) VV
    - Chave mestre (iniciar ação)
    - Cartão bloqueado (está armazenado no banco de dados de cartões inválidos)
    - Chave mestre (salvar ação)
    - Sinal sonoro (confirmando o comando ou indicando falha)
    
- Desbloquear sistema VV
    - Segura botão RESET
    - Chave mestre (iniciar ação) - durante
    - Segura botão RESET (15 segundos após chave mestre)
    - led indicador apaga
    - Chave mestre (salvar ação)
    - led indicador acende
    - Sinal sonoro (confirmando o comando ou indicando falha)
    
- Resetar o sistema VV
    - Apaga todos os dados salvos no arduino usando combinação de botao e chave
    - Sinais sonoros e led


## Anotações