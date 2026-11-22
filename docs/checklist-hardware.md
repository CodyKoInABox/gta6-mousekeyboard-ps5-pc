# Checklist de testes no hardware real

Para o seu PC, o PS5 e a capturadora Ripsaw com o Veyra. Marque cada item e anote o que aconteceu.
Os testes que capturam o teclado e o mouse são curtos: **Esc** solta tudo e desliga a captura a qualquer momento.

Ambiente: PC Windows 10 (build 19045), PS5 na mesma rede, Veyra NRVideo instalado.

## A. Instalação

- [ ] Hash SHA-256 do instalador bate com `SHA256SUMS.txt`.
- [ ] O aviso do SmartScreen aparece e "Executar assim mesmo" abre o instalador.
- [ ] A instalação não pede senha de administrador (pasta `%LOCALAPPDATA%\Programs\PS5KeyboardBridge`).
- [ ] Atalhos no menu Iniciar e na área de trabalho aparecem, e abrem o app.
- [ ] O pacote portátil (zip) abre o app sem instalar.

## B. Assistente de configuração

- [ ] O passo "Boas-vindas" mostra "Windows compatível.".
- [ ] **Procurar PS5** encontra o PS5 com o PS5 ligado.
- [ ] **Procurar PS5** encontra o PS5 em repouso (com "Ligar pela rede" ativo).
- [ ] Digitar o IP do PS5 em "Não apareceu?" e clicar em **Usar este IP** também escolhe o console.
- [ ] Os passos Remote Play e Conta PSN mostram as instruções corretas, em português.
- [ ] **Abrir login da Sony** abre o site da Sony no navegador padrão.
- [ ] Depois do login, colar a URL "redirect" e clicar em **Concluir login** reconhece a conta.
- [ ] O app nunca pede a senha da conta PSN.
- [ ] Um código de PIN errado é recusado com uma mensagem clara.
- [ ] Um código de PIN novo (Link Device) pareia o PS5. O app mostra "Pareado com ...".
- [ ] **Conectar e testar** conecta ao PS5, e o botão X é pressionado no PS5 (confirme na tela do PS5).
- [ ] **Procurar automaticamente** acha o Veyra.exe, ou **Escolher Veyra.exe...** funciona.
- [ ] "Pronto para jogar" aparece no final e **Concluir** fecha o assistente.

## C. Pareamento e configurações salvas

- [ ] Depois de reiniciar o PC, o app ainda mostra "PS5 pareado" e não pede novo pareamento.
- [ ] Depois de reiniciar, as configurações (perfil, Veyra, tecla F8, opções) continuam.
- [ ] **Esquecer PS5** apaga o pareamento; a tela principal volta para "ainda não pareado".

## D. Jogar (captura e entrada)

Teste com o jogo aberto, no Veyra em tela cheia se possível.

- [ ] **JOGAR** conecta; o status vai de Conectando a Conectado.
- [ ] Com o Veyra em foco e F8 ativado, a captura fica "ativa". Sem o Veyra em foco, fica "aguardando".
- [ ] W A S D andam no PS5 (analógico esquerdo).
- [ ] Mover o mouse mira no PS5 (analógico direito).
- [ ] Clique esquerdo e direito fazem R2 e L2 (mirar e atirar, no jogo).
- [ ] Espaço, Ctrl, E, R, Q, F, Tab, Enter, Backspace e as setas chegam ao PS5 como no mapa de controles do guia.
- [ ] Enquanto a captura está ativa, Espaço, Tab, Enter e cliques **não** acionam nada no Veyra (a janela não muda, nada de tela cheia sair).
- [ ] O cursor fica preso à janela do Veyra enquanto a captura está ativa.
- [ ] Alt+Tab, Win e Ctrl+Alt+Del continuam funcionando durante a captura.
- [ ] **Esc** solta todos os botões e desarma a captura. O cursor volta a ficar livre.
- [ ] F8 liga e desliga a captura, sem enviar o F8 para o jogo.
- [ ] Trocar de janela no meio de uma andada solta os botões: o personagem para.
- [ ] **PARAR** desconecta e o status volta para Desconectado.

## E. Conexão e recuperação

- [ ] Desligar o Wi-Fi do PC por ~10 s durante o jogo: o app mostra Conectando e volta a Conectado sozinho.
- [ ] Desligar o PS5 (ou mandá-lo para o repouso) durante o jogo: o app mostra erro ou tentativa de reconexão, sem travar.
- [ ] Ligar o PS5 de novo e clicar em **JOGAR**: conecta de novo.
- [ ] Sessão de 30 minutos: sem queda, sem entrada travada, sem vazamento de tela (verifique o gerenciador de tarefas).
- [ ] Com "Reconectar automaticamente" desligado, a queda vira erro e não reconecta sozinha.

## F. Bandeja e janela

- [ ] Fechar a janela (X) com "Minimizar para a bandeja" ligado: o app vai para a bandeja e continua rodando.
- [ ] Clique duplo no ícone da bandeja reabre a janela.
- [ ] Menu da bandeja: **Abrir**, **Jogar/Parar** e **Sair** funcionam.
- [ ] Abrir o app de novo com ele já aberto traz a janela existente para frente (não abre uma segunda cópia).

## G. Perfis e mira

- [ ] Perfil "Dirigir (volante suave)": W/S viram gatilhos e A/D suavizam o volante.
- [ ] Ajustes avançados de mira: a sensibilidade X e Y muda a mira de forma independente.
- [ ] Zona morta alta faz o analógico parar com movimentos pequenos.

## H. Veyra

- [ ] Com **Abrir o Veyra ao jogar** ligado, o Veyra é aberto (ou trazido para a frente) ao clicar em **JOGAR**.
- [ ] O Veyra continua mostrando a imagem da capturadora durante o jogo.
- [ ] O app não altera o arquivo de configuração do Veyra (confira que nada mudou).
- [ ] Sem o Veyra instalado, o app ainda conecta e joga; só não abre o Veyra.

## I. Instalação, atualização e desinstalação

- [ ] Instalar uma versão nova por cima da antiga mantém o pareamento e as configurações.
- [ ] Desinstalar pelo Windows (Configurações > Aplicativos) remove o app e mantém `%LOCALAPPDATA%\PS5KeyboardBridge`.
- [ ] Reinstalar depois de desinstalar reconecta sem novo pareamento.

## J. Rede e firewall

- [ ] Na primeira busca de rede, o Firewall do Windows pede permissão. Anote qual rede você escolheu.
- [ ] Com a VPN ligada, a conexão falha com mensagem clara (ou funciona, se a VPN deixar o tráfego local passar). Anote o resultado.

## K. Outros Remote Play

- [ ] Com o PS Remote Play aberto no PC, JOGAR mostra o aviso "Outro app de Remote Play está aberto". (Disponível a partir da versão com a verificação de conflito.)
- [ ] Com outro cliente conectado ao PS5, a conexão falha com "Outro Remote Play ... já está conectado". Feche o outro app e a conexão volta.

## Relatório

Ao final, envie:

1. Quais itens passaram, falharam ou não puderam ser testados.
2. Para cada falha: o horário, o que você fez e o que apareceu na tela (mensagem exata do app).
3. Versão do PS5, firmware e modelo da capturadora.
